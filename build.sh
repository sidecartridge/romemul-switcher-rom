#!/bin/sh
#
# build.sh: the release build of the three rescue ROM images. Each image is
# built out of tree by tools/dev/build.sh (pinned toolchain, build ID, shared
# finalizer; EPIC-00 STORY-01) and its published files are copied to
# dist/<platform>/.
#
# Debug and test images never come from here and never reach dist/:
#   tools/dev/build.sh <st|ste|amiga> <debug|test|debug-test>
#
# The firmware pins the images in dist/ by SHA-256 (C-06): tell it before a
# rebuild lands there.
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
cd "${SCRIPT_DIR}"

usage() {
  echo "Usage: ./build.sh [st|ste|amiga|all]" >&2
  echo "Debug and test images: tools/dev/build.sh <st|ste|amiga> <debug|test|debug-test>" >&2
}

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

case "$1" in
  all)
    "$0" st
    "$0" ste
    "$0" amiga
    exit 0
    ;;
  st) SIZE=192 PRG=RSWIT192.PRG ;;
  ste) SIZE=256 PRG=RSWIT256.PRG ;;
  amiga) SIZE=512 PRG= ;;
  *)
    usage
    exit 1
    ;;
esac
PLATFORM=$1

VERSION="$(tr -d '\r\n' < version.txt | sed 's/^[vV]//')"
IMAGE="RESCUE_SWITCHER_v${VERSION}_${SIZE}KB.img"
OUT="tools/dev/builds/${PLATFORM}-release"

tools/dev/build.sh "${PLATFORM}" release

mkdir -p "dist/${PLATFORM}"
cp "${OUT}/${IMAGE}" "dist/${PLATFORM}/${IMAGE}"
if [ -n "${PRG}" ]; then
  cp "${OUT}/ROMSWITC.PRG" "dist/${PLATFORM}/${PRG}"
fi
echo "published: dist/${PLATFORM}/${IMAGE}"
