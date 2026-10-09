#!/usr/bin/env python3
"""
File: tools/dev/fsuae_harness.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: Boot the debug+test Amiga image as the Kickstart of an A500 in
FS-UAE, type on the chooser and read its trace through Paula's serial port, and
print PASS or FAIL (EPIC-00 STORY-04).

Usage: fsuae_harness.py [--corrupt] [--no-warp] [--interactive]

Build the image first: tools/dev/build.sh amiga debug-test. Each run gets a
folder under tools/dev/logs/ with the trace, FS-UAE's configuration and logs.
FS-UAE has no way to inject keys or take a screenshot from outside, so keys and
trace both go through the serial port, mapped to a pseudo-terminal.
"""

import argparse
import os
import pty
import select
import signal
import subprocess
import sys
import tty

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness_common as hc  # noqa: E402

# One byte per key, as the debug build's src/amiga/kbd.c reads them.
KEY_BYTES = {"up": b"\x80", "down": b"\x81", "left": b"\x82", "right": b"\x83",
             "return": b"\r", "esc": b"\x1b", "any": b"x"}
KEY_TIMEOUT = 4.0


def fsuae_config(image, serial, run_dir, args):
    lines = [
        "[fs-uae]",
        "amiga_model = A500",
        f"kickstart_file = {image}",
        "chip_memory = 512",  # 512 KB and nothing else (D-07)
        "slow_memory = 0",
        "fast_memory = 0",
        f"serial_port = {serial}",
        f"logs_dir = {run_dir}",
        "audio_driver = dummy",
        "sound_output = none",
        "volume = 0",
    ]
    if not args.interactive:
        lines.append("window_hidden = 1")
        if not args.no_warp:
            lines.append("warp_mode = 1")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("Usage:")[0])
    parser.add_argument("--corrupt", action="store_true",
                        help="flip one payload byte: the ROM checksum must fail")
    parser.add_argument("--no-warp", action="store_true",
                        help="run at the A500's own speed")
    parser.add_argument("--interactive", action="store_true",
                        help="open a window and hand over; no keys, no PASS/FAIL")
    args = parser.parse_args()

    try:
        image = hc.image_path("amiga")
        fsuae = hc.emulator_path("fsuae")
    except hc.HarnessError as err:
        hc.die(str(err))
    label = "fsuae-amiga-a500" + ("-corrupt" if args.corrupt else "") + \
        ("-nowarp" if args.no_warp else "") + ("-interactive" if args.interactive else "")
    run_dir = hc.new_run_dir(label)
    if args.corrupt:
        image = hc.corrupt_copy(image, os.path.join(run_dir, "corrupt.img"))
        hc.fix_kickstart_checksum(image)  # FS-UAE boots it; our check fails

    # The serial port: a pseudo-terminal in raw mode before FS-UAE opens it, or
    # a key byte waits for a newline (sidecartos-config EPIC-00 STORY-04).
    master, slave = pty.openpty()
    tty.setraw(slave)
    tty.setraw(master)
    cfg_path = os.path.join(run_dir, "harness.fs-uae")
    with open(cfg_path, "w", encoding="utf-8") as handle:
        handle.write(fsuae_config(image, os.ttyname(slave), run_dir, args))

    proc = subprocess.Popen([fsuae, cfg_path],
                            stdout=open(os.path.join(run_dir, "fs-uae.out"), "wb"),
                            stderr=subprocess.STDOUT)

    def read_more():
        ready, _, _ = select.select([master], [], [], 0)
        return os.read(master, 4096) if ready else b""

    trace = hc.Trace(read_more)
    steps = hc.Steps()
    print(f"{label}: {os.path.relpath(image, hc.REPO)}", flush=True)

    if args.interactive:
        print("FS-UAE is open; close it to end the run.", flush=True)
        while proc.poll() is None:
            trace.wait_for(r"$^", 0.5)
        return hc.finish(label, run_dir, steps, None, trace)

    def shot(name):
        return None  # FS-UAE takes no screenshot from outside (see STORY-04)

    def press(key, expect, consume=True):
        for attempt in range(2):
            os.write(master, KEY_BYTES[key])
            if trace.wait_for(expect, KEY_TIMEOUT, consume):
                return
            steps.retries += 1
        steps.fail(f"key {key}", f"no {expect!r} after two presses")

    error = None
    try:
        hc.run_session(trace, press, shot, steps, "amiga", args.corrupt)
    except (hc.HarnessError, OSError) as err:
        error = str(err)
    finally:
        proc.send_signal(signal.SIGTERM)  # FS-UAE takes it cleanly
        try:
            proc.wait(10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(5)
        trace.wait_for(r"$^", 0)
        os.close(master)
        os.close(slave)
    if proc.poll() is None:
        error = (error or "") + "; FS-UAE still running"
    return hc.finish(label, run_dir, steps, error, trace)


if __name__ == "__main__":
    sys.exit(main())
