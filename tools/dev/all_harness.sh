#!/usr/bin/env bash
#
# tools/dev/all_harness.sh: build the three release and debug+test images and run every
# harness session on them, one line per run, then an overall PASS or FAIL
# (EPIC-00 STORY-05). Sequential: the ST and STE runs share no state, but the
# machine's CPU is the limit anyway.
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here/../.."

failed=0
mkdir -p tools/dev/logs
# The release images too: trace-only code compiles out there, and a release
# build broke once without any session noticing (EPIC-03 STORY-07).
for platform in st ste amiga; do
  for type in release debug-test; do
    log="tools/dev/logs/build-$platform-$type.log"
    if ! tools/dev/build.sh "$platform" "$type" > "$log" 2>&1; then
      echo "FAIL build $platform $type (see $log)"
      failed=1
    fi
  done
done
[[ $failed -eq 0 ]] || { echo "FAIL: a build failed, no session run"; exit 1; }

runs=(
  "hatari --image st"
  "hatari --image st --monitor mono"
  "hatari --image st --machine megast"
  "hatari --image st --corrupt"
  "hatari --image ste"
  "hatari --image ste --machine megaste"
  "hatari --image ste --monitor mono"
  "hatari --image st --stuck-line 16"
  "hatari --image ste --stuck-line 3"
  "hatari --image st --stuck-data 9"
  "hatari --image st --soak"
  "hatari --image ste --stress-fault 5"
  "hatari --image st --stress-alias 14"
  "hatari --image st --memsize 1024"
  "hatari --image ste --memsize 4096"
  "hatari --image ste --machine megaste --memsize 2048"
  "fsuae"
  "fsuae --corrupt"
  "fsuae --stuck-line 18"
  "fsuae --stuck-data 0"
  "fsuae --soak"
  "fsuae --stress-fault 12"
  "fsuae --stress-alias 18"
  "fsuae --chip 1024 --slow 512"
  "fsuae --chip 2048 --slow 1536"
)
for run in "${runs[@]}"; do
  read -r emulator flags <<< "$run"
  line="$(python3 "tools/dev/${emulator}_harness.py" $flags 2>&1 | tail -1)"
  echo "${line/ (*\/tools\/dev\/logs\// (logs/}"
  [[ "$line" == PASS* ]] || failed=1
done

if [[ $failed -eq 0 ]]; then
  echo "PASS: ${#runs[@]} runs"
else
  echo "FAIL: at least one run failed"
fi
exit $failed
