#!/usr/bin/env bash
# tools/dev/romtool.sh: run the pinned romtool (amitools 0.8.1) on files inside
# the repository. Paths are relative to the repository root.
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
source "$here/toolchain.sh"
ensure_romtool_image
docker run --rm --platform linux/x86_64 -v "$repo:/work" -w /work \
  --user "$(id -u):$(id -g)" "$ROMTOOL_IMAGE" "$@"
