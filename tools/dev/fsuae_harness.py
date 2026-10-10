#!/usr/bin/env python3
"""
File: tools/dev/fsuae_harness.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: Boot the debug+test Amiga image as the Kickstart of an A500 in
FS-UAE, type on the chooser and read its trace through Paula's serial port, and
print PASS or FAIL (EPIC-00 STORY-04).

Usage: fsuae_harness.py [--chip KB] [--slow KB] [--corrupt] [--no-warp] [--interactive]

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
             "return": b"\r", "esc": b"\x1b", "any": b"x", "t": b"t", "s": b"s"}
KEY_TIMEOUT = 4.0


def fsuae_config(image, serial, run_dir, args):
    lines = [
        "[fs-uae]",
        "amiga_model = A500",
        f"kickstart_file = {image}",
        f"chip_memory = {args.chip}",  # 512 KB and nothing else by default (D-07)
        f"slow_memory = {args.slow}",
        "fast_memory = 0",
        f"serial_port = {serial}",
        f"logs_dir = {run_dir}",
        # With no joystick connected, FS-UAE makes the keyboard a joystick in
        # port 1: the cursor keys and right Alt/Ctrl never reach the Amiga
        # keyboard. The chooser needs the arrows (EPIC-01).
        "joystick_port_1 = none",
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
    parser.add_argument("--chip", type=int, choices=[512, 1024, 2048], default=512,
                        help="chip RAM in KB: the self-test's RAM probe must find it")
    parser.add_argument("--slow", type=int, choices=[0, 512, 1024, 1536], default=0,
                        help="slow RAM at 0xC00000 in KB: the RAM probe must find it")
    parser.add_argument("--corrupt", action="store_true",
                        help="flip one payload byte: the ROM checksum must fail")
    parser.add_argument("--stuck-line", type=int, metavar="N",
                        help="make address line AN read as stuck: the self-test must say so")
    parser.add_argument("--stuck-data", type=int, metavar="K",
                        help="make data line DK read as stuck at 0: the self-test must say so")
    parser.add_argument("--stress-fault", type=int, metavar="K",
                        help="flip data line DK across the stress region: the stress reads must say so")
    parser.add_argument("--stress-alias", type=int, metavar="N",
                        help="make address line AN read as stuck across the stress region")
    parser.add_argument("--soak", action="store_true",
                        help="also run the 30-second soak from the self-test screen")
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
        (f"-chip{args.chip}" if args.chip != 512 else "") + \
        (f"-slow{args.slow}" if args.slow else "") + \
        (f"-stuck-a{args.stuck_line}" if args.stuck_line else "") + \
        (f"-stuck-d{args.stuck_data}" if args.stuck_data is not None else "") + \
        (f"-stress-d{args.stress_fault}" if args.stress_fault is not None else "") + \
        (f"-stress-a{args.stress_alias}" if args.stress_alias is not None else "") + \
        ("-soak" if args.soak else "") + \
        ("-nowarp" if args.no_warp else "") + ("-interactive" if args.interactive else "")
    run_dir = hc.new_run_dir(label)
    if args.corrupt:
        image = hc.corrupt_copy(image, os.path.join(run_dir, "corrupt.img"))
        hc.fix_kickstart_checksum(image)  # FS-UAE boots it; our check fails
    if args.stuck_line:
        image = hc.stuck_line_copy(image, os.path.join(run_dir, "stuck.img"),
                                   "amiga", args.stuck_line)
    if args.stuck_data is not None:
        image = hc.stuck_data_copy(image, os.path.join(run_dir, "stuck-data.img"),
                                   "amiga", args.stuck_data)
    if args.stress_fault is not None:
        image = hc.stress_fault_copy(image, os.path.join(run_dir, "stress-fault.img"),
                                     "amiga", args.stress_fault)
    if args.stress_alias is not None:
        image = hc.stress_alias_copy(image, os.path.join(run_dir, "stress-alias.img"),
                                     "amiga", args.stress_alias)

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
        print("FS-UAE is open; close it (or press Ctrl-C here) to end the run.", flush=True)
        try:
            while proc.poll() is None:
                trace.drain(0.5)
        except KeyboardInterrupt:
            proc.send_signal(signal.SIGTERM)
            try:
                proc.wait(10)
            except subprocess.TimeoutExpired:
                proc.kill()
        trace.drain(0.5)
        return hc.finish(label, run_dir, steps, None, trace, interactive=True)

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
        hc.run_session(trace, press, shot, steps, "amiga", args.corrupt,
                       stuck_line=args.stuck_line, stuck_data=args.stuck_data,
                       stress_fault=args.stress_fault, soak=args.soak,
                       stress_alias=args.stress_alias,
                       machine="amiga", ram=(args.chip, args.slow))
    except (hc.HarnessError, OSError) as err:
        error = str(err)
    finally:
        proc.send_signal(signal.SIGTERM)  # FS-UAE takes it cleanly
        try:
            proc.wait(10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(5)
        trace.drain(0.5)
        os.close(master)
        os.close(slave)
    if proc.poll() is None:
        error = (error or "") + "; FS-UAE still running"
    return hc.finish(label, run_dir, steps, error, trace)


if __name__ == "__main__":
    sys.exit(main())
