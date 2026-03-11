#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
cd "${SCRIPT_DIR}"

usage() {
  echo "Usage: ./build.sh [st|ste|amiga|all] [debug] [test]" >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

TARGET_PLATFORM="$1"
shift

if [ "${TARGET_PLATFORM}" = "all" ]; then
  "$0" st "$@"
  "$0" ste "$@"
  "$0" amiga "$@"
  exit 0
fi

case "${TARGET_PLATFORM}" in
  ste)
    ROM_BASE_ADDR_UL_VALUE="0x00E00000UL"
    STARTUP_ROM_ASM_VALUE="startup_ste.s"
    ROM_IMAGE_SIZE_SUFFIX="_256KB"
    TARGET_ROM_IMAGE_SIZE_BYTES=262144
    DIST_PRG_NAME="RSWIT256.PRG"
    ;;
  st)
    ROM_BASE_ADDR_UL_VALUE="0x00FC0000UL"
    STARTUP_ROM_ASM_VALUE="startup_st.s"
    ROM_IMAGE_SIZE_SUFFIX="_192KB"
    TARGET_ROM_IMAGE_SIZE_BYTES=196608
    DIST_PRG_NAME="RSWIT192.PRG"
    ;;
  amiga)
    ROM_BASE_ADDR_UL_VALUE="0x00F80000UL"
    ROM_IMAGE_SIZE_SUFFIX="_512KB"
    BUILD_TARGET="amiga"
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

pad_file_to_exact_size() {
  file_path="$1"
  target_size_bytes="$2"
  if [ ! -f "${file_path}" ]; then
    return 0
  fi
  size_bytes="$(wc -c < "${file_path}" | tr -d '[:space:]')"
  if [ "${size_bytes}" -gt "${target_size_bytes}" ]; then
    echo "Error: ${file_path} is larger than target size ${target_size_bytes} bytes" >&2
    exit 1
  fi
  if [ "${size_bytes}" -eq "${target_size_bytes}" ]; then
    return 0
  fi
  pad_bytes=$((target_size_bytes - size_bytes))
  dd if=/dev/zero bs="${pad_bytes}" count=1 >> "${file_path}" 2>/dev/null
}

publish_dist_artifacts() {
  dist_dir="dist/${TARGET_PLATFORM}"
  mkdir -p "${dist_dir}"

  if [ "${TARGET_PLATFORM}" = "amiga" ]; then
    rm -f "${dist_dir}/${DIST_ROM_IMAGE_NAME}" \
      "${dist_dir}"/RESCUE_SWITCHER_v*_512KB_*.img
    cp "build/amiga/${SOURCE_ROM_IMAGE_NAME}" "${dist_dir}/${DIST_ROM_IMAGE_NAME}"
  else
    rm -f "${dist_dir}/ROMSWITC.PRG" "${dist_dir}/RESCUE_SWITCHER_v${APP_VERSION}.img"
    cp build/st/ROMSWITC.PRG "${dist_dir}/${DIST_PRG_NAME}"
    cp "build/st/${SOURCE_ROM_IMAGE_NAME}" "${dist_dir}/${DIST_ROM_IMAGE_NAME}"
  fi
}

selected_build_dir() {
  if [ "${TARGET_PLATFORM}" = "amiga" ]; then
    if [ "${DEBUG_MODE}" = "1" ] && [ "${TEST_MODE}" = "1" ]; then
      echo "build/amiga-debug-test"
    elif [ "${DEBUG_MODE}" = "1" ]; then
      echo "build/amiga-debug"
    elif [ "${TEST_MODE}" = "1" ]; then
      echo "build/amiga-test"
    else
      echo "build/amiga"
    fi
  else
    if [ "${DEBUG_MODE}" = "1" ] && [ "${TEST_MODE}" = "1" ]; then
      echo "build/st-debug-test"
    elif [ "${DEBUG_MODE}" = "1" ]; then
      echo "build/st-debug"
    elif [ "${TEST_MODE}" = "1" ]; then
      echo "build/st-test"
    else
      echo "build/st"
    fi
  fi
}

SELECTED_BUILD_DIR="$(selected_build_dir)"

if [ "${DEBUG_MODE}" = "0" ] && [ "${TEST_MODE}" = "0" ]; then
  if [ "${TARGET_PLATFORM}" = "amiga" ]; then
    rm -f "build/amiga/${SOURCE_ROM_IMAGE_NAME}" build/amiga/ROMSWAMI.MAP
  else
    rm -f build/st/ROMSWITC.PRG build/st/ROMSWITC.BIN build/st/ROMSWITC.MAP \
      "build/st/${SOURCE_ROM_IMAGE_NAME}" build/st/ROMSWROM.BIN build/st/ROMSWROM.MAP
  fi
fi

if [ "${TARGET_PLATFORM}" = "amiga" ]; then
  STCMD_NO_TTY=1 ST_WORKING_FOLDER="${SCRIPT_DIR}" stcmd make amiga DEBUG="${DEBUG_MODE}" TEST="${TEST_MODE}" ROM_BASE_ADDR_UL="${ROM_BASE_ADDR_UL_VALUE}"
else
  STCMD_NO_TTY=1 ST_WORKING_FOLDER="${SCRIPT_DIR}" stcmd make st DEBUG="${DEBUG_MODE}" TEST="${TEST_MODE}" ROM_BASE_ADDR_UL="${ROM_BASE_ADDR_UL_VALUE}" STARTUP_ROM_ASM="${STARTUP_ROM_ASM_VALUE}"
fi

publish_rom_artifacts_if_present() {
  src_dir="$1"
  if [ -f "${src_dir}/${SOURCE_ROM_IMAGE_NAME}" ]; then
    cp "${src_dir}/${SOURCE_ROM_IMAGE_NAME}" "build/st/${SOURCE_ROM_IMAGE_NAME}"
    pad_file_to_exact_size "build/st/${SOURCE_ROM_IMAGE_NAME}" \
      "${TARGET_ROM_IMAGE_SIZE_BYTES}"
  fi
  if [ -f "${src_dir}/ROMSWROM.MAP" ]; then
    cp "${src_dir}/ROMSWROM.MAP" build/st/ROMSWROM.MAP
  fi
}

if [ "${TARGET_PLATFORM}" = "amiga" ]; then
  mkdir -p build/amiga
  if [ "${SELECTED_BUILD_DIR}" != "build/amiga" ]; then
    cp "${SELECTED_BUILD_DIR}/${SOURCE_ROM_IMAGE_NAME}" "build/amiga/${SOURCE_ROM_IMAGE_NAME}"
    if [ -f "${SELECTED_BUILD_DIR}/ROMSWAMI.MAP" ]; then
      cp "${SELECTED_BUILD_DIR}/ROMSWAMI.MAP" build/amiga/ROMSWAMI.MAP
    elif [ -f "${SELECTED_BUILD_DIR}/ROMAMDBG.MAP" ]; then
      cp "${SELECTED_BUILD_DIR}/ROMAMDBG.MAP" build/amiga/ROMSWAMI.MAP
    fi
  fi

  romtool copy -c "build/amiga/${SOURCE_ROM_IMAGE_NAME}" "build/amiga/${SOURCE_ROM_IMAGE_NAME}.tmp"
  mv "build/amiga/${SOURCE_ROM_IMAGE_NAME}.tmp" "build/amiga/${SOURCE_ROM_IMAGE_NAME}"
else
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
fi

if [ "${TARGET_PLATFORM}" = "amiga" ]; then
  romtool info "build/amiga/${SOURCE_ROM_IMAGE_NAME}"
else
  if [ "${TEST_MODE}" = "0" ]; then
    pad_file_to_exact_size "build/st/${SOURCE_ROM_IMAGE_NAME}" \
      "${TARGET_ROM_IMAGE_SIZE_BYTES}"
  fi
fi
if [ "${TEST_MODE}" = "0" ]; then
  publish_dist_artifacts
fi
