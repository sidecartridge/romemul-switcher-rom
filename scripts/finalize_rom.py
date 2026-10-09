#!/usr/bin/env python3
"""
File: scripts/finalize_rom.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: Turn a linked ROM image into the published one: exact size,
random fill in the unused space, and the 32-bit big-endian additive checksum
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


def fill_random(image, offset, length):
    if length > 0:
        image[offset:offset + length] = os.urandom(length)


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
    image.extend(os.urandom(p["size"] - len(image)))
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
    fill_random(image, payload_end, p["kickety"] - payload_end)
    fill_random(image, kickety_end, p["check"] - kickety_end)
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
