#!/usr/bin/env bash
#
# tools/dev/build.sh: build one rescue ROM image out of tree, in the pinned
# toolchain image, and finalize it (EPIC-00 STORY-01). Modelled on
# sidecartos-config's tools/dev/build.sh.
#
# Usage: tools/dev/build.sh <st|ste|amiga> <release|debug|test|debug-test|screenshot>
#
# Output goes to tools/dev/builds/<platform>-<type>/ (git-ignored): the
# finalized image under its published name, the ST/STE program, the link maps,
# and the objects under obj/. build/ and dist/ are never touched.
#
# screenshot: the release code with a cartridge that always answers and a key
# script (src/common/screenshot.c), for the user documentation's captures in
# Hatari and FS-UAE. It shows the build ID of the release tagged v<version>,
# as the published images do, and is named SCREENSHOT_v<ver>_<size>KB.img so
# it is not taken for one of them. Never for hardware.
set -euo pipefail

usage() {
  echo "usage: $0 <st|ste|amiga> <release|debug|test|debug-test|screenshot>" >&2
  exit 2
}
[[ $# -eq 2 ]] || usage
platform=$1
type=$2

case "$type" in
  release) debug=0 test=0 screenshot=0 ;;
  debug) debug=1 test=0 screenshot=0 ;;
  test) debug=0 test=1 screenshot=0 ;;
  debug-test) debug=1 test=1 screenshot=0 ;;
  screenshot) debug=0 test=0 screenshot=1 ;;
  *) usage ;;
esac

case "$platform" in
  st) subdir=st base=0x00FC0000UL startup=startup_st.s size=192 ;;
  ste) subdir=st base=0x00E00000UL startup=startup_ste.s size=256 ;;
  amiga) subdir=amiga base=0x00F80000UL startup= size=512 ;;
  *) usage ;;
esac

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
cd "$repo"
source "$here/toolchain.sh"

version="$(tr -d '\r\n' < version.txt | sed 's/^[vV]//')"
build_id="$(rescue_build_id "$repo" "$type")"
prefix=RESCUE_SWITCHER
if [[ "$screenshot" == 1 ]]; then
  build_id="$(git -C "$repo" rev-parse --short=7 "v$version^{commit}")"
  prefix=SCREENSHOT
fi
out="tools/dev/builds/$platform-$type"   # relative to the repository root
echo "build id: $build_id"

make_args="DEBUG=$debug TEST=$test SCREENSHOT=$screenshot ROM_BASE_ADDR_UL=$base BUILD_ID=$build_id BUILD_DIR=/work/$out/obj"
[[ -n "$startup" ]] && make_args+=" STARTUP_ROM_ASM=$startup"

# The folder is emptied inside the container, on the same side of Docker's
# file sharing as the compiler (sidecartos-config EPIC-04 STORY-03).
docker run --rm --platform linux/x86_64 -v "$repo:/work" -w /work \
  --user "$(id -u):$(id -g)" --entrypoint bash "$ATARI_IMAGE" \
  -c "rm -rf ./$out && mkdir -p ./$out/obj && make -s -C src/$subdir $make_args"

raw="$out/obj/RESCUE_SWITCHER_v$version.img"
image="$out/${prefix}_v${version}_${size}KB.img"
map=-
if [[ "$platform" == "amiga" ]]; then
  map="$out/obj/ROMSWAMI.MAP"
  [[ "$debug" == 1 ]] && map="$out/obj/ROMAMDBG.MAP"
fi

# The finalizer runs in the romtool image (Python and romtool), not on the
# host: a file the host writes into a folder a container has just recreated
# stays invisible to the next container for a while (Docker's file sharing),
# so every step that writes and then reads runs on the container side.
ensure_romtool_image
docker run --rm --platform linux/x86_64 -v "$repo:/work" -w /work \
  --user "$(id -u):$(id -g)" --entrypoint python3 "$ROMTOOL_IMAGE" \
  scripts/finalize_rom.py "$platform" "$raw" "$map" "$image" --romtool romtool

if [[ "$platform" != "amiga" ]]; then
  prg="$out/obj/ROMSWITC.PRG"
  [[ "$debug" == 1 ]] && prg="$out/obj/ROMSWDBG.PRG"
  cp "$prg" "$out/ROMSWITC.PRG"
fi
cp "$out"/obj/*.MAP "$out/"
echo "built: $image"
