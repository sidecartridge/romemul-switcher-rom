"""
File: tools/dev/harness_common.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: What the Hatari and FS-UAE harnesses share (EPIC-00): the
configuration, the values the session expects (read from src/common/test.c),
the run folder, the trace reader and the step record. Standard library only.
"""

import configparser
import os
import re
import shutil
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "scripts"))
import finalize_rom  # noqa: E402  (the image layout, written once)

DEFAULT_PATHS = {
    "hatari": "/usr/local/bin/hatari",
    "fsuae": "/Applications/FS-UAE.app/Contents/MacOS/fs-uae",
}

IMAGE_SIZES_KB = {"st": 192, "ste": 256, "amiga": 512}


class HarnessError(Exception):
    pass


def emulator_path(name):
    """The emulator binary from tools/dev/dev.cfg ([emulators]), else the default."""
    path = DEFAULT_PATHS[name]
    cfg_path = os.path.join(HERE, "dev.cfg")
    if os.path.exists(cfg_path):
        cfg = configparser.ConfigParser()
        cfg.read(cfg_path)
        path = cfg.get("emulators", name, fallback=path)
    if not os.access(path, os.X_OK):
        raise HarnessError(
            f"{name} not found at {path}; set it in tools/dev/dev.cfg "
            "(see dev.cfg.example)")
    return path


def version():
    with open(os.path.join(REPO, "version.txt"), encoding="utf-8") as handle:
        return handle.read().strip().lstrip("vV")


def image_path(platform):
    """The debug+test image tools/dev/build.sh makes for this platform."""
    path = os.path.join(
        REPO, "tools", "dev", "builds", f"{platform}-debug-test",
        f"RESCUE_SWITCHER_v{version()}_{IMAGE_SIZES_KB[platform]}KB.img")
    if not os.path.exists(path):
        raise HarnessError(
            f"{path} is missing; build it with: tools/dev/build.sh {platform} debug-test")
    return path


def expectations():
    """What the debug+test build must show, read from the sources it was built from."""
    with open(os.path.join(REPO, "src", "common", "test.c"), encoding="utf-8") as handle:
        src = handle.read()

    def array(name):
        match = re.search(r"flash" + name + r"Raw\[[^\]]*\]\s*=\s*\{(.*?)\};", src, re.S)
        if not match:
            raise HarnessError(f"flash{name}Raw not found in src/common/test.c")
        return bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))

    catalog = array("Catalog")
    params = array("Params")
    count = 0
    while count * 256 < len(catalog) and catalog[count * 256] != 0:
        count += 1
    names = [catalog[i * 256:i * 256 + 64].split(b"\0")[0].decode("latin-1")
             for i in range(count)]
    with open(os.path.join(REPO, "src", "common", "chooser.c"), encoding="utf-8") as handle:
        page_size = int(re.search(r"kElementsPerPage\s*=\s*(\d+)", handle.read()).group(1))
    def u32(at):
        return int.from_bytes(params[at:at + 4], "little")

    busflags, romflags, rescue = u32(524), u32(532), u32(528)
    config = {  # as the emulator reads them (EPIC-03 STORY-08)
        "ticks": u32(516), "oe": int(u32(520) != 0),
        "busflags": 0 if busflags == 0xFFFFFFFF else busflags,
        "rescue": 0 if rescue > 600 else rescue,
        "romflags": 0 if romflags == 0xFFFFFFFF else romflags,
    }
    return {
        "config": config,
        "count": count,
        "names": names,
        "default": params[70] | (params[71] << 8),
        "rescue": params[326] | (params[327] << 8),
        "protocol": params[512] | (params[513] << 8),
        "page_size": page_size,
        "pages": (count + page_size - 1) // page_size,
        "version": version(),
    }


def new_run_dir(label):
    stamp = time.strftime("%Y%m%d-%H%M%S")
    path = os.path.join(HERE, "logs", f"{stamp}-{label}")
    os.makedirs(path)
    return path


def corrupt_copy(src, dst):
    """A copy with one byte flipped where our ROM checksum covers it but no code
    runs: 32 bytes from the end is random fill on every image (the ST checksum
    is the last 4 bytes; the Amiga's sits 28 from the end, then the Kickstart
    checksum and footer). A flipped byte in code would make the run undefined:
    the first version flipped offset 0x1000 and broke the Amiga chooser."""
    shutil.copyfile(src, dst)
    offset = os.path.getsize(dst) - 32
    with open(dst, "r+b") as handle:
        handle.seek(offset)
        byte = handle.read(1)[0]
        handle.seek(offset)
        handle.write(bytes([byte ^ 0xFF]))
    return dst


def fix_kickstart_checksum(path):
    """Rewrite the Kickstart checksum (the long 24 bytes from the end of a
    512 KB image) as romtool's `copy -c` does: the sum of every long, with
    end-around carry, must be 0xFFFFFFFF. Our own checksum field is untouched,
    so a corrupted copy still boots as a Kickstart and fails our check."""
    with open(path, "r+b") as handle:
        data = bytearray(handle.read())
        offset = len(data) - 24
        data[offset:offset + 4] = b"\0\0\0\0"
        total = 0
        for i in range(0, len(data), 4):
            total += int.from_bytes(data[i:i + 4], "big")
            if total > 0xFFFFFFFF:
                total = (total & 0xFFFFFFFF) + 1
        data[offset:offset + 4] = ((~total) & 0xFFFFFFFF).to_bytes(4, "big")
        handle.seek(0)
        handle.write(data)


class Trace:
    """The ROM's trace as it arrives, searched forward from the last match."""

    def __init__(self, read_more):
        self._read_more = read_more  # returns new bytes, or b"" when none yet
        self.data = b""
        self.pos = 0

    def wait_for(self, pattern, timeout, consume=True):
        regex = re.compile(pattern.encode())
        end = time.time() + timeout
        while True:
            match = regex.search(self.data, self.pos)
            if match:
                if consume:
                    self.pos = match.end()
                return match
            if time.time() >= end:
                return None
            more = self._read_more()
            if more:
                self.data += more
            else:
                time.sleep(0.02)

    def drain(self, seconds):
        """Read whatever arrives for this long, matching nothing."""
        end = time.time() + seconds
        while True:
            more = self._read_more()
            if more:
                self.data += more
            elif time.time() >= end:
                return
            else:
                time.sleep(0.02)

    def lines(self):
        return [line for line in self.data.decode("latin-1").splitlines()
                if line.startswith("@@")]


class Steps:
    """Step-by-step PASS/FAIL record, printed as the session goes."""

    def __init__(self):
        self.done = 0
        self.retries = 0
        self.start = time.time()

    def ok(self, name):
        self.done += 1
        print(f"  {time.time() - self.start:6.1f}s  ok    {name}", flush=True)

    def fail(self, name, why):
        print(f"  {time.time() - self.start:6.1f}s  FAIL  {name}: {why}", flush=True)
        raise HarnessError(f"{name}: {why}")

    def elapsed(self):
        return time.time() - self.start


# Where each image sits, and the RAM the harnesses give each machine by default:
# 512 KB in one part, Hatari's bank 0 or FS-UAE's chip RAM (D-07).
ROM_BASES = {"st": 0xFC0000, "ste": 0xE00000, "amiga": 0xF80000}
DEFAULT_RAM = (512, 0)


def selftest_lines(platform, stuck_line=None, stuck_data=None, machine=None,
                   screen=None, stress_fault=None, stress_alias=None, ram=DEFAULT_RAM):
    """The trace lines the self-test (EPIC-03) must print between its start and
    end lines, in order, as (step name, pattern). Each test story adds its own.
    ram: the two parts the RAM probe must find, in KB (STORY-13)."""
    lines = []
    size = finalize_rom.PLATFORMS[platform]["size"]
    lines.append(("rescue image", rf"@@ selftest image size={size >> 10} "
                  rf"base={ROM_BASES[platform]:06x}\n"))
    for n in range(1, finalize_rom.address_line_count(size) + 1):
        state = "stuck" if n == stuck_line else "ok"
        lines.append((f"A{n} {state}", rf"@@ selftest addr A{n} {state}\n"))
    if stuck_data is None:
        lines.append(("data lines ok", r"@@ selftest data ok\n"))
    else:
        lines.append((f"D{stuck_data} stuck at 0",
                      rf"@@ selftest data stuck0={1 << stuck_data:04x} stuck1=0000 "
                      r"shorted=0000 lanes=0\n"))
    lines.append(("stress reads", stress_pattern("selftest stress", stress_fault,
                                                 alias=stress_alias)))
    # No emulator answers a command (C-03): every ping must say so, quickly.
    for i in range(1, 9):
        lines.append((f"ping {i} no answer", rf"@@ selftest ping {i} noanswer\n"))
    lines.append(("ping summary", r"@@ selftest ping answered=0 sent=8\n"))
    lines.append(("reliability skipped", r"@@ selftest reliability skipped\n"))
    lines.append(("flash reads skipped", r"@@ selftest flash skipped\n"))
    exp, cfg = expectations(), expectations()["config"]
    lines.append(("catalog ok", rf"@@ selftest catalog ok entries={exp['count']} "
                  rf"protocol=0x{exp['protocol']:04x} default={exp['default']} "
                  rf"rescue={exp['rescue']} badnames=0 badsizes=0\n"))
    lines.append(("configuration", rf"@@ selftest config ticks={cfg['ticks']} "
                  rf"oe={cfg['oe']} busflags=0x{cfg['busflags']:x} "
                  rf"rescue={cfg['rescue']} romflags=0x{cfg['romflags']:x}\n"))
    if machine == "amiga":
        # FS-UAE's A500: an OCS Agnus, PAL (ID 0x00), or an ECS one (0x20) when
        # it is given more than the 512 KB of chip RAM an OCS Agnus can address.
        agnus = 0x00 if ram[0] <= 512 else 0x20
        lines.append((f"machine amiga, Agnus 0x{agnus:02x}",
                      rf"@@ selftest machine amiga chip=0x{agnus:02x} denise=0x[0-9a-f]{{2}}\n"))
    elif machine is not None:
        lines.append((f"machine {machine}",
                      rf"@@ selftest machine {machine} chip=0x00 denise=0x00\n"))
        mode = "mono" if screen == "mono" else "colour"
        lines.append((f"monitor {mode} stable",
                      rf"@@ selftest monitor {mode} changes=0 samples=\d+\n"))
    lines.append((f"RAM {ram[0]}+{ram[1]} KB",
                  rf"@@ selftest ram total={ram[0] + ram[1]} parts={ram[0]}\+{ram[1]}\n"))
    return lines


def stuck_data_copy(src, dst, platform, k):
    """A copy where data line Dk reads as stuck at 0: bit k cleared in every
    data pattern word, checksums recomputed (EPIC-03 STORY-04)."""
    p = finalize_rom.PLATFORMS[platform]
    with open(src, "rb") as handle:
        image = bytearray(handle.read())
    for offset, word in finalize_rom.data_patterns(p["size"]):
        image[offset:offset + 2] = struct.pack(">H", word & ~(1 << k) & 0xFFFF)
    finalize_rom.write_check_field(image, p)
    with open(dst, "wb") as handle:
        handle.write(image)
    if platform == "amiga":
        fix_kickstart_checksum(dst)
    return dst


def stress_pattern(prefix, fault=None, seconds=None, alias=None):
    """The trace line of a stress pass or a soak: no error; with --stress-fault K,
    errors on data line K only; with --stress-alias N, on address line N only
    (EPIC-03 STORY-11)."""
    head = rf"@@ {prefix} " + (rf"seconds={seconds} " if seconds is not None else "")
    if fault is not None:
        return head + rf"reads=\d+ errors=[1-9]\d* data={1 << fault:04x} addr=00000"
    if alias is not None:
        return head + rf"reads=\d+ errors=[1-9]\d* data=0000 addr={1 << alias:05x}"
    return head + r"reads=\d+ errors=0 data=0000 addr=00000"


def stress_reserved(size):
    """The self-test's own words in the stress region, which the ROM never
    stress-reads (mirrors stressReserved() for the offsets the build uses)."""
    reserved = set()
    for _, ref, _, partner, _ in finalize_rom.address_layout(size):
        reserved.update((ref, partner))
    kickety = size // 2
    reserved.update(range(kickety, kickety + 8, 2))
    return reserved


def stress_alias_copy(src, dst, platform, n):
    """A copy where address line An reads as stuck at 0 across the stress region:
    every word whose offset has that line set holds the word of the offset with it
    clear, checksums recomputed (EPIC-03 STORY-11)."""
    p = finalize_rom.PLATFORMS[platform]
    size = p["size"]
    reserved = stress_reserved(size)
    with open(src, "rb") as handle:
        image = bytearray(handle.read())
    for offset in range(finalize_rom.STRESS_START, size - finalize_rom.STRESS_TOP_GAP, 2):
        if offset & (1 << n) and offset not in reserved:
            struct.pack_into(">H", image, offset, finalize_rom.fill_word(offset ^ (1 << n)))
    finalize_rom.write_check_field(image, p)
    with open(dst, "wb") as handle:
        handle.write(image)
    if platform == "amiga":
        fix_kickstart_checksum(dst)
    return dst


def stress_fault_copy(src, dst, platform, k):
    """A copy where data line Dk reads flipped all over the stress region: bit k
    of every pattern word there changed, the self-test's own words left alone,
    checksums recomputed (EPIC-03 STORY-11)."""
    p = finalize_rom.PLATFORMS[platform]
    size = p["size"]
    reserved = stress_reserved(size)
    with open(src, "rb") as handle:
        image = bytearray(handle.read())
    for offset in range(finalize_rom.STRESS_START, size - finalize_rom.STRESS_TOP_GAP, 2):
        if offset not in reserved:
            word = struct.unpack_from(">H", image, offset)[0] ^ (1 << k)
            struct.pack_into(">H", image, offset, word)
    finalize_rom.write_check_field(image, p)
    with open(dst, "wb") as handle:
        handle.write(image)
    if platform == "amiga":
        fix_kickstart_checksum(dst)
    return dst


def stuck_line_copy(src, dst, platform, n):
    """A copy where address line An reads as stuck: its partner word holds the
    reference's signature. Our checksum, and on the Amiga the Kickstart one,
    are recomputed, so only the self-test sees the fault (EPIC-03 STORY-03)."""
    p = finalize_rom.PLATFORMS[platform]
    with open(src, "rb") as handle:
        image = bytearray(handle.read())
    _, _, ref_sig, partner, _ = finalize_rom.address_layout(p["size"])[n - 1]
    image[partner:partner + 2] = struct.pack(">H", ref_sig)
    finalize_rom.write_check_field(image, p)
    with open(dst, "wb") as handle:
        handle.write(image)
    if platform == "amiga":
        fix_kickstart_checksum(dst)
    return dst


def run_session(trace, press, shot, steps, platform, corrupt, screen=None,
                stuck_line=None, stuck_data=None, machine=None, stress_fault=None,
                soak=False, stress_alias=None, ram=DEFAULT_RAM):
    """The scripted session, the same on every emulator (EPIC-00 STORY-03, STORY-04).

    press(key, expect, consume) sends one key ("down", "up", "left", "right",
    "return", "esc" or "any") and waits for the trace line it must produce;
    shot(name) saves a screenshot when the emulator can and returns its path, or
    None. Every expected value
    comes from src/common/test.c and chooser.c.
    """
    exp = expectations()
    ver = re.escape(exp["version"])
    names = exp["names"]

    def need(name, pattern, timeout=8):
        if not trace.wait_for(pattern, timeout):
            path = shot(f"fail-{name.replace(' ', '-')}")
            steps.fail(name, f"no {pattern!r} in the trace"
                       + (f"; screenshot {path}" if path else ""))
        steps.ok(name)

    need("boot", rf"@@ rescue v{ver} \S+ {platform}\n", 90)
    if screen:
        need(f"screen {screen}", rf"@@ screen {screen}\n", 30)
        # The resolution set in the vertical blank, not on a timeout, and the
        # STE's extra video registers found only on an STE (EPIC-03 STORY-14).
        ste = 1 if machine in ("ste", "megaste") else 0
        need("screen set in the vertical blank", rf"@@ screen vbl ok ste={ste}\n")
    if corrupt:
        need("romcheck fail", r"@@ romcheck fail .*\n@@ waitkey\n", 90)
        shot("romcheck-fail")
        press("any", r"@@ list ", False)
    else:
        need("romcheck ok", r"@@ romcheck ok ", 90)
    # No emulator answers (C-03): the boot ping says so, and the test image
    # carries on with its built-in list (EPIC-03 STORY-10).
    need("boot ping no answer", r"@@ bootping noanswer\n")
    need("list", rf"@@ list {exp['count']} entries default={exp['default']} "
                 rf"rescue={exp['rescue']}\n")
    need("protocol", rf"@@ protocol 0x{exp['protocol']:04x}\n")
    need("page 1 drawn", rf"@@ page 1 of {exp['pages']} drawn\n")
    need("waiting at entry 0", r"@@ waitkey index 0 page 1\n")
    shot("list")

    # The self-test (EPIC-03): T opens it, ESC returns to the list.
    press("t", r"@@ selftest start\n")
    start = trace.pos
    for name, pattern in selftest_lines(platform, stuck_line, stuck_data,
                                        machine, screen, stress_fault, stress_alias,
                                        ram):
        need(f"self-test {name}", pattern, 60)
    # The ping, the reliability count and the flash reads are skipped in a
    # test build; only the injected fault, if any, fails.
    faults = sum(x is not None for x in (stuck_line, stuck_data, stress_fault,
                                         stress_alias))
    need("self-test end", rf"@@ selftest end passed=\d+ failed={faults} skipped=3\n", 60)
    # Every frame of the self-test carries its own nonce (EPIC-03 STORY-01). In
    # an emulator that is one frame per ping: with no answer base+0 never
    # changes, so no restore frame is sent.
    nonces = re.findall(rb"@@ nonce ([0-9a-f]{8})\n", trace.data[start:trace.pos])
    if len(nonces) < 8 or len(set(nonces)) != len(nonces):
        steps.fail("self-test nonces", f"{len(nonces)} frames, "
                   f"{len(set(nonces))} distinct nonces")
    steps.ok(f"self-test nonces ({len(nonces)} frames, all distinct)")
    need("self-test waits", r"@@ waitkey\n")
    shot("self-test")
    if soak:
        # S: the 30-second soak, in emulated frames (EPIC-03 STORY-11).
        press("s", r"@@ soak start\n")
        need("soak, 30 s", stress_pattern("soak end", stress_fault, 30, stress_alias)
             + r" stopped=0\n", 300)
        need("waiting after the soak", r"@@ waitkey\n")
        shot("soak")
    press("esc", rf"@@ page 1 of {exp['pages']} drawn\n", False)
    need("list after the self-test", rf"@@ page 1 of {exp['pages']} drawn\n")
    need("waiting at entry 0 after the self-test", r"@@ waitkey index 0 page 1\n")

    press("down", r"@@ key 0x50 index 0\n")
    need("cursor on entry 1", r"@@ waitkey index 1 page 1\n")
    if exp["pages"] > 1:
        press("right", r"@@ key 0x4d index 1\n")
        need("page 2 drawn", rf"@@ page 2 of {exp['pages']} drawn\n")
        need("cursor on page 2", rf"@@ waitkey index {exp['page_size']} page 2\n")
        shot("page-2")
        press("left", rf"@@ key 0x4b index {exp['page_size']}\n")
        need("page 1 again", rf"@@ page 1 of {exp['pages']} drawn\n")
        need("cursor back on entry 0", r"@@ waitkey index 0 page 1\n")
    else:
        press("up", r"@@ key 0x48 index 1\n")
        need("cursor back on entry 0", r"@@ waitkey index 0 page 1\n")

    press("return", r"@@ key 0x1c index 0\n")
    need("confirm entry 0", rf"@@ confirm 0 {re.escape(names[0])}\n@@ waitkey\n")
    shot("confirm")
    press("esc", r"@@ select cancelled\n")
    need("list after cancel", rf"@@ page 1 of {exp['pages']} drawn\n")
    need("waiting at entry 0 again", r"@@ waitkey index 0 page 1\n")

    press("down", r"@@ key 0x50 index 0\n")
    need("cursor on entry 1 again", r"@@ waitkey index 1 page 1\n")
    press("return", r"@@ key 0x1c index 1\n")
    need("confirm entry 1", rf"@@ confirm 1 {re.escape(names[1])}\n@@ waitkey\n")
    press("any", rf"@@ select 1 {re.escape(names[1])}\n")
    need("reset", r"@@ reset rom 1\n")
    need("boot after reset", rf"@@ rescue v{ver} \S+ {platform}\n", 90)
    # No text ran past column 80 on any screen (EPIC-03 STORY-12).
    overflow = re.search(rb"@@ overflow row \d+\n", trace.data)
    if overflow:
        steps.fail("80 columns", f"text ran past column 80: {overflow.group().decode().strip()}")
    steps.ok("80 columns")


def finish(label, run_dir, steps, error, trace, interactive=False):
    with open(os.path.join(run_dir, "trace.txt"), "w", encoding="latin-1") as handle:
        handle.write("\n".join(trace.lines()) + "\n")
    if interactive:
        # Nothing was checked: say so rather than PASS.
        print(f"ENDED {label}: interactive run, {len(trace.lines())} trace lines "
              f"({run_dir})")
        return 0
    if error is None:
        print(f"PASS {label}: {steps.done} steps, {steps.retries} key retries, "
              f"{steps.elapsed():.0f} s ({run_dir})")
        return 0
    print(f"FAIL {label}: {error} ({run_dir})")
    return 1


def die(message):
    print(f"error: {message}", file=sys.stderr)
    sys.exit(2)
