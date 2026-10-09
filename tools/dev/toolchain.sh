# tools/dev/toolchain.sh: sourced by every build script. The pinned images and
# the build ID, kept in one place (EPIC-00 STORY-01). The same arrangement as
# sidecartos-config's tools/dev/toolchain.sh.

# All three images build with the Atari GCC toolchain (GCC 4.6.4, binutils
# 2.30): Diego's atarist-toolkit-docker, pinned by tag and digest. The tag is
# the one sidecartos-config pins.
ATARI_IMAGE="logronoide/atarist-toolkit-docker-x86_64:1.4.0@sha256:016434131cf53f627c365e55262845fba53f30a7de87a1f122d58f8fa6aeeebe"
# romtool (amitools 0.8.1, as sidecartos-config pins it) on Python 3.12: runs
# scripts/finalize_rom.py, which needs romtool for the Kickstart checksum and the
# check of the Amiga image. Built from tools/dev/docker/romtool.
ROMTOOL_IMAGE="romemul-switcher-rom/romtool:1"

# rescue_build_id <repo> <release|debug|test|debug-test>: <sha7>, then
# -dirty.<diff7> when src/, the makefiles or version.txt differ from HEAD, then
# +debug and +test. Computed on the host and passed to make as BUILD_ID, so it
# does not depend on git inside a container.
rescue_build_id() {
  local repo=$1 type=$2 id paths
  paths=(src Makefile version.txt)
  id="$(git -C "$repo" rev-parse --short=7 HEAD 2>/dev/null || echo unknown)"
  if ! git -C "$repo" diff --quiet HEAD -- "${paths[@]}" 2>/dev/null; then
    id+="-dirty.$(git -C "$repo" diff HEAD -- "${paths[@]}" | shasum | cut -c1-7)"
  fi
  case "$type" in
    debug) id+="+debug" ;;
    test) id+="+test" ;;
    debug-test) id+="+debug+test" ;;
  esac
  printf '%s' "$id"
}

# ensure_romtool_image: build the romtool image when it is missing.
ensure_romtool_image() {
  docker image inspect "$ROMTOOL_IMAGE" >/dev/null 2>&1 \
    || docker build -q --platform linux/x86_64 -t "$ROMTOOL_IMAGE" \
         "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/docker/romtool" >/dev/null
}
