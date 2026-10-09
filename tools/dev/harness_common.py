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
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

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
    return {
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


def run_session(trace, press, shot, steps, platform, corrupt, screen=None):
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
    if corrupt:
        need("romcheck fail", r"@@ romcheck fail .*\n@@ waitkey\n", 90)
        shot("romcheck-fail")
        press("any", r"@@ list ", False)
    else:
        need("romcheck ok", r"@@ romcheck ok ", 90)
    need("list", rf"@@ list {exp['count']} entries default={exp['default']} "
                 rf"rescue={exp['rescue']}\n")
    need("protocol", rf"@@ protocol 0x{exp['protocol']:04x}\n")
    need("page 1 drawn", rf"@@ page 1 of {exp['pages']} drawn\n")
    need("waiting at entry 0", r"@@ waitkey index 0 page 1\n")
    shot("list")

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


def finish(label, run_dir, steps, error, trace):
    with open(os.path.join(run_dir, "trace.txt"), "w", encoding="latin-1") as handle:
        handle.write("\n".join(trace.lines()) + "\n")
    if error is None:
        print(f"PASS {label}: {steps.done} steps, {steps.retries} key retries, "
              f"{steps.elapsed():.0f} s ({run_dir})")
        return 0
    print(f"FAIL {label}: {error} ({run_dir})")
    return 1


def die(message):
    print(f"error: {message}", file=sys.stderr)
    sys.exit(2)
