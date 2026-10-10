#!/usr/bin/env bash
#
# tools/dev/measure_builds.sh: build every platform as release and debug-test
# and print the payload and free space of each image as a Markdown table, for a
# story's before and after numbers (EPIC-00 STORY-05).
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here/../.."

echo "| image | type | payload (bytes) | free (bytes) |"
echo "| --- | --- | --- | --- |"
for platform in st ste amiga; do
  for type in release debug-test; do
    line="$(tools/dev/build.sh "$platform" "$type" 2>&1 | grep -E "^$platform: payload")"
    payload="$(sed -E 's/.*payload ([0-9]+) bytes.*/\1/' <<< "$line")"
    free="$(sed -E 's/.*bytes, ([0-9]+) free.*/\1/' <<< "$line")"
    echo "| \`$platform\` | $type | $payload | $free |"
  done
done
