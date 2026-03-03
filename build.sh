#!/bin/sh
set -eu

usage() {
  echo "Usage: ./build.sh [st|ste|amiga] [debug] [test]" >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

TARGET_PLATFORM="$1"
shift

case "${TARGET_PLATFORM}" in
  ste)
    ROM_BASE_ADDR_UL_VALUE="0x00E00000UL"
    STARTUP_ROM_ASM_VALUE="startup_ste.s"
    ROM_IMAGE_SIZE_SUFFIX="_256KB"
    DIST_PRG_NAME="RSWIT256.PRG"
    ;;
  st)
    ROM_BASE_ADDR_UL_VALUE="0x00FC0000UL"
    STARTUP_ROM_ASM_VALUE="startup_st.s"
    ROM_IMAGE_SIZE_SUFFIX="_192KB"
    DIST_PRG_NAME="RSWIT192.PRG"
    ;;
  amiga)
    echo "amiga build is not implemented yet" >&2
    exit 1
    ;;
  *)
    usage
    exit 1
    ;;
esac

DEBUG_MODE="${DEBUG:-0}"
TEST_MODE="${TEST:-0}"
for arg in "$@"; do
  case "${arg}" in
    debug)
      DEBUG_MODE=1
      ;;
    test)
      TEST_MODE=1
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

if [ "${DEBUG_MODE}" = "1" ]; then
  DEBUG_MODE=1
else
  DEBUG_MODE=0
fi

if [ "${TEST_MODE}" = "1" ]; then
  TEST_MODE=1
else
  TEST_MODE=0
fi

APP_VERSION="$(if [ -f version.txt ]; then tr -d '\r\n' < version.txt; else echo 0.0.0; fi)"
SOURCE_ROM_IMAGE_NAME="RESCUE_SWITCHER_v${APP_VERSION}.img"
DIST_ROM_IMAGE_NAME="RESCUE_SWITCHER_v${APP_VERSION}${ROM_IMAGE_SIZE_SUFFIX}.img"

pad_file_to_4k() {
  file_path="$1"
  if [ ! -f "${file_path}" ]; then
    return 0
  fi
  size_bytes="$(wc -c < "${file_path}" | tr -d '[:space:]')"
  remainder=$((size_bytes % 4096))
  if [ "${remainder}" -eq 0 ]; then
    return 0
  fi
  pad_bytes=$((4096 - remainder))
  dd if=/dev/zero bs="${pad_bytes}" count=1 >> "${file_path}" 2>/dev/null
}

publish_dist_artifacts() {
  dist_dir="dist/${TARGET_PLATFORM}"
  mkdir -p "${dist_dir}"

  rm -f "${dist_dir}/ROMSWITC.PRG" "${dist_dir}/RESCUE_SWITCHER_v${APP_VERSION}.img"
  cp build/st/ROMSWITC.PRG "${dist_dir}/${DIST_PRG_NAME}"
  cp "build/st/${SOURCE_ROM_IMAGE_NAME}" "${dist_dir}/${DIST_ROM_IMAGE_NAME}"
}

# In release mode, remove potentially copied debug/test artifacts first.
# Otherwise `make` may think build/st/ROMSWITC.* is up-to-date and skip rebuild.
if [ "${DEBUG_MODE}" = "0" ] && [ "${TEST_MODE}" = "0" ]; then
  rm -f build/st/ROMSWITC.PRG build/st/ROMSWITC.BIN build/st/ROMSWITC.MAP \
    "build/st/${SOURCE_ROM_IMAGE_NAME}" build/st/ROMSWROM.BIN build/st/ROMSWROM.MAP
fi

STCMD_NO_TTY=1 ST_WORKING_FOLDER=/Users/diego/mister_wkspc/romemul-switcher-rom stcmd make st DEBUG="${DEBUG_MODE}" TEST="${TEST_MODE}" ROM_BASE_ADDR_UL="${ROM_BASE_ADDR_UL_VALUE}" STARTUP_ROM_ASM="${STARTUP_ROM_ASM_VALUE}"

publish_rom_artifacts_if_present() {
  src_dir="$1"
  if [ -f "${src_dir}/${SOURCE_ROM_IMAGE_NAME}" ]; then
    cp "${src_dir}/${SOURCE_ROM_IMAGE_NAME}" "build/st/${SOURCE_ROM_IMAGE_NAME}"
    pad_file_to_4k "build/st/${SOURCE_ROM_IMAGE_NAME}"
  fi
  if [ -f "${src_dir}/ROMSWROM.MAP" ]; then
    cp "${src_dir}/ROMSWROM.MAP" build/st/ROMSWROM.MAP
  fi
}

# Publish the selected build artifact to build/st/ROMSWITC.* so runtime tools
# that always load that path execute the chosen mode (release/debug/test).
if [ "${DEBUG_MODE}" = "1" ] && [ "${TEST_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-debug-test/ROMSWDBG.PRG build/st/ROMSWITC.PRG
  cp build/st-debug-test/ROMSWDBG.BIN build/st/ROMSWITC.BIN
  cp build/st-debug-test/ROMSWDBG.MAP build/st/ROMSWITC.MAP
  publish_rom_artifacts_if_present build/st-debug-test
elif [ "${DEBUG_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-debug/ROMSWDBG.PRG build/st/ROMSWITC.PRG
  cp build/st-debug/ROMSWDBG.BIN build/st/ROMSWITC.BIN
  cp build/st-debug/ROMSWDBG.MAP build/st/ROMSWITC.MAP
  publish_rom_artifacts_if_present build/st-debug
elif [ "${TEST_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-test/ROMSWITC.PRG build/st/ROMSWITC.PRG
  cp build/st-test/ROMSWITC.BIN build/st/ROMSWITC.BIN
  cp build/st-test/ROMSWITC.MAP build/st/ROMSWITC.MAP
  publish_rom_artifacts_if_present build/st-test
fi

pad_file_to_4k "build/st/${SOURCE_ROM_IMAGE_NAME}"
if [ "${TEST_MODE}" = "0" ]; then
  publish_dist_artifacts
fi
