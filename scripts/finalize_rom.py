#!/usr/bin/env python3
"""
File: scripts/finalize_rom.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: Turn a linked ROM image into the published one: exact size, a
computed pattern in the unused space, the self-test's words, and the 32-bit
big-endian additive checksum
the ROM verifies at boot. On the Amiga also the Kickstart checksum, written
and checked with romtool.

Moved out of build.sh's inline Python so build.sh and tools/dev/build.sh
finalize every image the same way (EPIC-00 STORY-01). Standard library only.

Usage: finalize_rom.py <st|ste|amiga> <raw image> <map file|-> <out image>
                       [--romtool CMD]

CMD runs romtool, e.g. tools/dev/romtool.sh; it is called from the repository
root with repository-relative paths. Required for the Amiga.
"""

import os
import re
import shlex
import shutil
import struct
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Sizes and offsets, as build.sh had them.
PLATFORMS = {
    "st": {"size": 196608, "check": 196604, "ignored": 0, "ignored_size": 0},
    "ste": {"size": 262144, "check": 262140, "ignored": 0, "ignored_size": 0},
    "amiga": {
        "size": 524288,
        "check": 524260,  # our checksum, just before the Kickstart footer
        "ignored": 524264,  # the Kickstart checksum, maintained by romtool
        "ignored_size": 4,
        "kickety": 262144,
        "kickety_size": 8,
    },
}


def fail(message):
    sys.exit(f"Error: {message}")


# The stress region of the self-test (EPIC-03 STORY-11): from here to 4 KB below
# the end of the image, every word is fill_word(offset) unless it is a self-test
# word. The payload must end before it.
STRESS_START = 0x10000
STRESS_TOP_GAP = 0x1000


def fill_word(offset):
    """The word the build writes at an even offset of the unused space, mixed
    from the offset so the ROM can check a read anywhere there. Shifts and
    additions, no multiply (the ROM has no libgcc), and not linear: with a
    xorshift the word one address line away always differed by the same bits,
    so a stuck address line could pass for a data line. Mirrored in
    src/common/selftest.c, selftestFillWord()."""
    m = 0xFFFFFFFF
    x = (offset ^ 0x9E3779B9) & m
    x = (x + (x << 10)) & m
    x ^= x >> 6
    x = (x + (x << 3)) & m
    x ^= x >> 11
    x = (x + (x << 15)) & m
    return (x ^ (x >> 16)) & 0xFFFF


def fill_pattern(image, offset, length):
    """The unused space, as computed words (EPIC-03 STORY-11): no longer random,
    so a clean build of a commit gives the same bytes anywhere."""
    for o in range(offset, offset + length):
        word = fill_word(o & ~1)
        image[o] = (word >> 8) if (o & 1) == 0 else (word & 0xFF)


def map_symbol(map_path, symbol):
    pattern = re.compile(
        r"^\s*(0x[0-9A-Fa-f]+)\s+" + re.escape(symbol) + r"(?:\s|$)")
    with open(map_path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            match = pattern.match(line)
            if match:
                return int(match.group(1), 16)
    fail(f"symbol {symbol} not found in {map_path}")
    return 0


def sum_be16(buf):
    total = 0
    length = len(buf)
    idx = 0
    while idx + 1 < length:
        total = (total + ((buf[idx] << 8) | buf[idx + 1])) & 0xFFFFFFFF
        idx += 2
    if idx < length:
        total = (total + (buf[idx] << 8)) & 0xFFFFFFFF
    return total


def address_line_count(size):
    """The address lines the image spans: A1 to A17 for 192 and 256 KB, A1 to A18
    for 512 KB (the 68000 has no A0)."""
    return (size - 1).bit_length() - 1


def address_layout(size):
    """The address-line test (EPIC-03 STORY-03): for each line An, a reference
    word and a partner word whose offsets differ only in that line, each with
    its signature, as (n, ref, ref_signature, partner, partner_signature).
    Mirrored in src/common/selftest.c, selftestAddressLayout()."""
    layout = []
    for n in range(1, address_line_count(size) + 1):
        ref, ref_sig = size - 0x100, 0x5AA5
        partner = ref ^ (1 << n)
        if partner >= size:
            # Only the ST's 192 KB window, line A16: a second reference low in
            # the image, clear of the first one's partners.
            ref, ref_sig = (size - 0x200) & 0xFFFF, 0x5A5A
            partner = ref ^ (1 << n)
        layout.append((n, ref, ref_sig, partner, 0xA500 | n))
    return layout


def data_patterns(size):
    """The data-line test (EPIC-03 STORY-04): a 64-byte block 4 KB from the end
    of the image holding the 16 walking-one words, then the 16 walking-zero
    words, as (offset, word). Mirrored in src/common/selftest.c."""
    block = size - 0x1000
    ones = [(block + 2 * k, 1 << k) for k in range(16)]
    zeros = [(block + 32 + 2 * k, 0xFFFF ^ (1 << k)) for k in range(16)]
    return ones + zeros


def write_selftest_patterns(image, p, payload_end):
    """Writes the self-test's words (address signatures, data patterns), refusing
    any offset in the payload, a checksum field, the Amiga's kickety split, or
    claimed twice."""
    size = p["size"]
    if payload_end > STRESS_START:
        fail(f"the payload ends at {payload_end:#x}, inside the self-test's stress region "
             f"(from {STRESS_START:#x})")
    forbidden = [(p["check"], size)]
    if "kickety" in p:
        forbidden.append((p["kickety"], p["kickety"] + p["kickety_size"]))
    words = []
    for n, ref, ref_sig, partner, partner_sig in address_layout(size):
        words += [(f"A{n}", ref, ref_sig), (f"A{n}", partner, partner_sig)]
    words += [("data", offset, word) for offset, word in data_patterns(size)]
    claimed = {}
    for what, offset, word in words:
        if offset < payload_end:
            fail(f"self-test word of {what} at {offset:#x} is inside the payload "
                 f"(ends at {payload_end:#x})")
        if any(lo <= offset + 1 and offset < hi for lo, hi in forbidden):
            fail(f"self-test word of {what} at {offset:#x} hits a reserved field")
        if claimed.get(offset, word) != word:
            fail(f"offset {offset:#x} claimed by two self-test words")
        claimed[offset] = word
        image[offset:offset + 2] = struct.pack(">H", word)


def write_check_field(image, p):
    check = p["check"]
    image[check:check + 4] = b"\x00\x00\x00\x00"
    checksum = sum_be16(image[:check])
    if p["ignored_size"] > 0:
        ignored = p["ignored"]
        checksum = (checksum + sum_be16(image[check + 4:ignored])) & 0xFFFFFFFF
        checksum = (checksum + sum_be16(
            image[ignored + p["ignored_size"]:])) & 0xFFFFFFFF
    else:
        checksum = (checksum + sum_be16(image[check + 4:])) & 0xFFFFFFFF
    image[check:check + 4] = struct.pack(">I", checksum)
    return checksum


def repo_path(path):
    """romtool runs in a container that mounts only the repository."""
    rel = os.path.relpath(os.path.abspath(path), REPO)
    if rel.startswith(".."):
        fail(f"{path} is outside the repository; romtool cannot reach it")
    return rel


def romtool(cmd, *args):
    subprocess.run(shlex.split(cmd) + list(args), cwd=REPO, check=True)


def finalize_st(platform, raw, out):
    p = PLATFORMS[platform]
    if len(raw) > p["check"]:
        fail(f"{platform} payload ({len(raw)} bytes) overlaps the checksum field")
    image = bytearray(raw)
    image.extend(bytes(p["size"] - len(image)))
    fill_pattern(image, len(raw), p["size"] - len(raw))
    write_selftest_patterns(image, p, len(raw))
    checksum = write_check_field(image, p)
    with open(out, "wb") as handle:
        handle.write(image)
    print(f"{platform}: payload {len(raw)} bytes, {p['check'] - len(raw)} free, "
          f"checksum {checksum:08X}")


def finalize_amiga(raw, map_path, out, cmd):
    p = PLATFORMS["amiga"]
    if cmd is None:
        fail("the Amiga image needs --romtool")
    if len(raw) != p["size"]:
        fail(f"amiga image is {len(raw)} bytes, expected {p['size']}")
    payload_end = map_symbol(map_path, "__rom_payload_end")
    kickety_end = p["kickety"] + p["kickety_size"]
    if payload_end > p["kickety"]:
        fail("Amiga payload overlaps the kickety split area")

    image = bytearray(raw)
    fill_pattern(image, payload_end, p["kickety"] - payload_end)
    fill_pattern(image, kickety_end, p["check"] - kickety_end)
    write_selftest_patterns(image, p, payload_end)
    image[p["check"]:p["check"] + 4] = b"\x00\x00\x00\x00"
    with open(out, "wb") as handle:
        handle.write(image)

    # Kickstart checksum, then ours (which excludes the Kickstart one), then
    # the Kickstart checksum again over our field: the order build.sh used.
    tmp = out + ".tmp"
    romtool(cmd, "copy", "-c", repo_path(out), repo_path(tmp))
    os.replace(tmp, out)
    with open(out, "rb") as handle:
        image = bytearray(handle.read())
    checksum = write_check_field(image, p)
    with open(out, "wb") as handle:
        handle.write(image)
    romtool(cmd, "copy", "-c", repo_path(out), repo_path(tmp))
    os.replace(tmp, out)
    romtool(cmd, "info", repo_path(out))
    print(f"amiga: payload {payload_end} bytes, {p['kickety'] - payload_end} free "
          f"before the kickety split, checksum {checksum:08X}")


def main(argv):
    args = list(argv[1:])
    cmd = None
    if "--romtool" in args:
        i = args.index("--romtool")
        if i + 1 >= len(args):
            fail("--romtool needs a command")
        cmd = args[i + 1]
        del args[i:i + 2]
    if len(args) != 4 or args[0] not in PLATFORMS:
        sys.exit(__doc__.split("Usage:")[1].strip().split("\n\n")[0])
    platform, raw_path, map_path, out = args
    with open(raw_path, "rb") as handle:
        raw = handle.read()
    if os.path.abspath(raw_path) != os.path.abspath(out):
        shutil.copyfile(raw_path, out)
    if platform == "amiga":
        finalize_amiga(raw, map_path, out, cmd)
    else:
        finalize_st(platform, raw, out)


if __name__ == "__main__":
    main(sys.argv)
