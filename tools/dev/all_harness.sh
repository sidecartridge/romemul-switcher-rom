#!/usr/bin/env bash
#
# tools/dev/all_harness.sh: build the three debug+test images and run every
# harness session on them, one line per run, then an overall PASS or FAIL
# (EPIC-00 STORY-05). Sequential: the ST and STE runs share no state, but the
# machine's CPU is the limit anyway.
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here/../.."

failed=0
mkdir -p tools/dev/logs
for platform in st ste amiga; do
  log="tools/dev/logs/build-$platform-debug-test.log"
  if ! tools/dev/build.sh "$platform" debug-test > "$log" 2>&1; then
    echo "FAIL build $platform (see $log)"
    failed=1
  fi
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
  "fsuae"
  "fsuae --corrupt"
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
