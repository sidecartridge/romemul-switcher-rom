#!/usr/bin/env python3
"""
File: tools/dev/hatari_harness.py
Author: Diego Parrilla Santamaría
Date: 2026-10-09
Copyright: 2024-26 - GOODDATA LABS SL
Description: Boot the debug+test ST or STE image as the TOS of Hatari, drive the
chooser with keys through Hatari's control socket, read the trace (NF_STDERR on
Hatari's stderr), and print PASS or FAIL (EPIC-00 STORY-03).

Usage: hatari_harness.py [--image st|ste] [--machine st|megast|ste|megaste]
                         [--monitor rgb|mono] [--corrupt] [--interactive]

Build the image first: tools/dev/build.sh <st|ste> debug-test. Each run gets a
folder under tools/dev/logs/ with the trace, Hatari's log and the screenshots.
"""

import argparse
import os
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness_common as hc  # noqa: E402

# ST scancodes (src/common/kbd.h); "any" is the space bar.
SCANCODES = {"up": "0x48", "down": "0x50", "left": "0x4b", "right": "0x4d",
             "return": "0x1c", "esc": "0x01", "any": "0x39"}
KEY_TIMEOUT = 4.0
# After a key's release, time for the ROM to read the break code before the next
# press: its ACIA poll is about 28 ms apart in a debug build, and a press while
# the release is unread overruns the ACIA and is lost (EPIC-00 STORY-02). 0.2 s
# of host time is seconds of emulated time under fast-forward.
RELEASE_PAUSE = 0.2


def hatari_args(image, args, run_dir, sock_path):
    cmd = [hc.emulator_path("hatari"), "--tos", image, "--machine", args.machine,
           "--memsize", "0", "--ttram", "0",  # 512 KB (D-07)
           "--monitor", args.monitor, "--natfeats", "yes", "--sound", "off",
           "--confirm-quit", "false", "--log-file", os.path.join(run_dir, "hatari.log")]
    if args.interactive:
        return cmd
    return cmd + ["--fast-forward", "yes", "--fast-forward-key-repeat", "false",
                  "--statusbar", "false", "--drive-led", "false",
                  "--control-socket", sock_path,
                  "--screenshot-dir", os.path.join(run_dir, "shots")]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("Usage:")[0])
    parser.add_argument("--image", choices=["st", "ste"], default="st")
    parser.add_argument("--machine", choices=["st", "megast", "ste", "megaste"])
    parser.add_argument("--monitor", choices=["rgb", "mono"], default="rgb")
    parser.add_argument("--corrupt", action="store_true",
                        help="flip one payload byte: the ROM checksum must fail")
    parser.add_argument("--interactive", action="store_true",
                        help="open a window and hand over; no keys, no PASS/FAIL")
    args = parser.parse_args()
    if args.machine is None:
        args.machine = "st" if args.image == "st" else "ste"
    if (args.image == "st") != (args.machine in ("st", "megast")):
        hc.die(f"the {args.image} image does not boot on a {args.machine}")

    try:
        image = hc.image_path(args.image)
    except hc.HarnessError as err:
        hc.die(str(err))
    label = f"hatari-{args.image}-{args.machine}-{args.monitor}" + \
        ("-corrupt" if args.corrupt else "") + ("-interactive" if args.interactive else "")
    run_dir = hc.new_run_dir(label)
    os.makedirs(os.path.join(run_dir, "shots"))
    os.makedirs(os.path.join(run_dir, "home"))
    if args.corrupt:
        image = hc.corrupt_copy(image, os.path.join(run_dir, "corrupt.img"))

    # A clean HOME: Hatari would otherwise load the user's saved configuration
    # (a GEMDOS drive, 4 MB of RAM). Headless unless interactive.
    env = dict(os.environ, HOME=os.path.join(run_dir, "home"))
    if not args.interactive:
        env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")

    # macOS caps a Unix socket path at 104 bytes; the run folder can be longer.
    sock_dir = tempfile.mkdtemp(prefix="hh", dir="/tmp")
    sock_path = os.path.join(sock_dir, "ctl.sock")
    server = None
    if not args.interactive:
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(sock_path)
        server.listen(1)
        server.settimeout(20)

    stderr_path = os.path.join(run_dir, "stderr.txt")
    stderr_file = open(stderr_path, "wb")
    proc = subprocess.Popen(hatari_args(image, args, run_dir, sock_path), env=env,
                            stdout=open(os.path.join(run_dir, "stdout.txt"), "wb"),
                            stderr=stderr_file)
    reader = open(stderr_path, "rb")
    trace = hc.Trace(reader.read)
    steps = hc.Steps()
    print(f"{label}: {os.path.relpath(image, hc.REPO)}", flush=True)

    if args.interactive:
        print("Hatari is open; close it (or press Ctrl-C here) to end the run.", flush=True)
        try:
            proc.wait()
        except KeyboardInterrupt:
            proc.send_signal(signal.SIGTERM)  # --confirm-quit false: no dialog
            try:
                proc.wait(10)
            except subprocess.TimeoutExpired:
                proc.kill()
        trace.drain(0.5)  # read what is left
        return hc.finish(label, run_dir, steps, None, trace, interactive=True)

    error = None
    conn = None
    try:
        conn, _ = server.accept()

        def command(text):
            conn.sendall((text + "\n").encode())

        def shot(name):
            shots = os.path.join(run_dir, "shots")
            before = set(os.listdir(shots))
            # Fast-forward skips rendering frames: give Hatari time to render
            # one after the ROM's last drawing, or the shot shows a stale frame.
            time.sleep(0.3)
            command("hatari-shortcut screenshot")
            end = time.time() + 5
            while time.time() < end:
                new = [f for f in os.listdir(shots) if f not in before]
                if new:
                    time.sleep(0.2)  # let Hatari finish writing it
                    path = os.path.join(shots, f"{name}.png")
                    os.replace(os.path.join(shots, new[0]), path)
                    return path
                time.sleep(0.05)
            return None

        def press(key, expect, consume=True):
            code = SCANCODES[key]
            for attempt in range(2):
                command(f"hatari-event keydown {code}")
                seen = trace.wait_for(expect, KEY_TIMEOUT, consume)
                command(f"hatari-event keyup {code}")
                time.sleep(RELEASE_PAUSE)
                if seen:
                    return
                steps.retries += 1
            path = shot(f"fail-key-{key}")
            steps.fail(f"key {key}", f"no {expect!r} after two presses"
                       + (f"; screenshot {path}" if path else ""))

        hc.run_session(trace, press, shot, steps, args.image, args.corrupt,
                       screen="mono" if args.monitor == "mono" else "medium")
    except (hc.HarnessError, socket.timeout, OSError) as err:
        error = str(err)
    finally:
        if conn is not None:
            try:
                conn.sendall(b"hatari-shortcut quit\n")
            except OSError:
                pass
        try:
            proc.wait(10)
        except subprocess.TimeoutExpired:
            proc.send_signal(signal.SIGKILL)
            proc.wait(5)
        trace.drain(0.5)
        shutil.rmtree(sock_dir, ignore_errors=True)
    if proc.poll() is None:
        error = (error or "") + "; Hatari still running"
    return hc.finish(label, run_dir, steps, error, trace)


if __name__ == "__main__":
    sys.exit(main())
