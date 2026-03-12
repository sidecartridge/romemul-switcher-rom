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
    CRC_FIELD_OFFSET_BYTES=262140
    SECOND_IGNORED_FIELD_OFFSET_BYTES=0
    SECOND_IGNORED_FIELD_SIZE_BYTES=0
    DIST_PRG_NAME="RSWIT256.PRG"
    ;;
  st)
    ROM_BASE_ADDR_UL_VALUE="0x00FC0000UL"
    STARTUP_ROM_ASM_VALUE="startup_st.s"
    ROM_IMAGE_SIZE_SUFFIX="_192KB"
    TARGET_ROM_IMAGE_SIZE_BYTES=196608
    CRC_FIELD_OFFSET_BYTES=196604
    SECOND_IGNORED_FIELD_OFFSET_BYTES=0
    SECOND_IGNORED_FIELD_SIZE_BYTES=0
    DIST_PRG_NAME="RSWIT192.PRG"
    ;;
  amiga)
    ROM_BASE_ADDR_UL_VALUE="0x00F80000UL"
    ROM_IMAGE_SIZE_SUFFIX="_512KB"
    TARGET_ROM_IMAGE_SIZE_BYTES=524288
    CRC_FIELD_OFFSET_BYTES=524260
    SECOND_IGNORED_FIELD_OFFSET_BYTES=524264
    SECOND_IGNORED_FIELD_SIZE_BYTES=4
    AMIGA_KICKETY_OFFSET_BYTES=262144
    AMIGA_KICKETY_SIZE_BYTES=8
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

RAW_APP_VERSION="$(if [ -f version.txt ]; then tr -d '\r\n' < version.txt; else echo 0.0.0; fi)"
case "${RAW_APP_VERSION}" in
  [vV]*)
    APP_VERSION="${RAW_APP_VERSION#?}"
    ;;
  *)
    APP_VERSION="${RAW_APP_VERSION}"
    ;;
esac
SOURCE_ROM_IMAGE_NAME="RESCUE_SWITCHER_v${APP_VERSION}.img"
DIST_ROM_IMAGE_NAME="RESCUE_SWITCHER_v${APP_VERSION}${ROM_IMAGE_SIZE_SUFFIX}.img"

fill_range_with_random_bytes() {
  file_path="$1"
  offset_bytes="$2"
  length_bytes="$3"
  if [ ! -f "${file_path}" ] || [ "${length_bytes}" -le 0 ]; then
    return 0
  fi
  python3 - "${file_path}" "${offset_bytes}" "${length_bytes}" <<'PY'
import os
import sys

path = sys.argv[1]
offset = int(sys.argv[2])
length = int(sys.argv[3])

with open(path, "r+b") as handle:
    handle.seek(offset)
    remaining = length
    while remaining > 0:
        chunk = min(remaining, 65536)
        handle.write(os.urandom(chunk))
        remaining -= chunk
PY
}

extend_file_with_random_bytes() {
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
  fill_range_with_random_bytes "${file_path}" "${size_bytes}" \
    "$((target_size_bytes - size_bytes))"
}

extract_map_symbol_value() {
  map_file_path="$1"
  symbol_name="$2"
  python3 - "${map_file_path}" "${symbol_name}" <<'PY'
import re
import sys

map_path = sys.argv[1]
symbol = sys.argv[2]
pattern = re.compile(r"^\s*(0x[0-9A-Fa-f]+)\s+" + re.escape(symbol) + r"(?:\s|$)")

with open(map_path, "r", encoding="utf-8", errors="replace") as handle:
    for line in handle:
        match = pattern.match(line)
        if match:
            print(int(match.group(1), 16))
            raise SystemExit(0)

raise SystemExit(f"Error: symbol {symbol} not found in {map_path}")
PY
}

write_check_field() {
  file_path="$1"
  target_size_bytes="$2"
  check_field_offset_bytes="$3"
  ignored_field_offset_bytes="$4"
  ignored_field_size_bytes="$5"
  python3 - "${file_path}" "${target_size_bytes}" "${check_field_offset_bytes}" \
    "${ignored_field_offset_bytes}" "${ignored_field_size_bytes}" <<'PY'
import struct
import sys

path = sys.argv[1]
target_size = int(sys.argv[2])
check_offset = int(sys.argv[3])
ignored_offset = int(sys.argv[4])
ignored_size = int(sys.argv[5])

def sum_be16(buf):
    total = 0
    length = len(buf)
    idx = 0
    while idx + 1 < length:
        total = (total + ((buf[idx] << 8) | buf[idx + 1])) & 0xFFFFFFFF
        idx += 2
    if idx < length:
        total = (total + (buf[idx] << 8)) & 0xFFFFFFFF
    return total

with open(path, "r+b") as handle:
    image = bytearray(handle.read())
    if len(image) != target_size:
        raise SystemExit(
            f"Error: {path} size {len(image)} does not match expected {target_size}"
        )
    if check_offset < 0 or (check_offset + 4) > len(image):
        raise SystemExit(f"Error: invalid checksum field offset {check_offset}")

    image[check_offset:check_offset + 4] = b"\x00\x00\x00\x00"
    checksum = sum_be16(image[:check_offset])
    if ignored_size > 0:
        checksum = (checksum + sum_be16(image[check_offset + 4:ignored_offset])) & 0xFFFFFFFF
        checksum = (checksum + sum_be16(image[ignored_offset + ignored_size:])) & 0xFFFFFFFF
    else:
        checksum = (checksum + sum_be16(image[check_offset + 4:])) & 0xFFFFFFFF

    handle.seek(0)
    handle.write(image)
    handle.seek(check_offset)
    handle.write(struct.pack(">I", checksum))
PY
}

finalize_st_rom_image() {
  file_path="$1"
  if [ ! -f "${file_path}" ]; then
    return 0
  fi
  size_bytes="$(wc -c < "${file_path}" | tr -d '[:space:]')"
  if [ "${size_bytes}" -gt "${CRC_FIELD_OFFSET_BYTES}" ]; then
    echo "Error: ${file_path} payload overlaps the checksum field" >&2
    exit 1
  fi
  extend_file_with_random_bytes "${file_path}" "${TARGET_ROM_IMAGE_SIZE_BYTES}"
  write_check_field "${file_path}" "${TARGET_ROM_IMAGE_SIZE_BYTES}" \
    "${CRC_FIELD_OFFSET_BYTES}" "${SECOND_IGNORED_FIELD_OFFSET_BYTES}" \
    "${SECOND_IGNORED_FIELD_SIZE_BYTES}"
}

finalize_amiga_rom_image() {
  file_path="$1"
  map_file_path="$2"
  if [ ! -f "${file_path}" ] || [ ! -f "${map_file_path}" ]; then
    return 0
  fi

  payload_end_bytes="$(extract_map_symbol_value "${map_file_path}" "__rom_payload_end")"
  file_size_bytes="$(wc -c < "${file_path}" | tr -d '[:space:]')"
  kickety_end_bytes=$((AMIGA_KICKETY_OFFSET_BYTES + AMIGA_KICKETY_SIZE_BYTES))

  if [ "${file_size_bytes}" -ne "${TARGET_ROM_IMAGE_SIZE_BYTES}" ]; then
    echo "Error: ${file_path} size ${file_size_bytes} does not match expected Amiga ROM size ${TARGET_ROM_IMAGE_SIZE_BYTES}" >&2
    exit 1
  fi

  if [ "${payload_end_bytes}" -gt "${AMIGA_KICKETY_OFFSET_BYTES}" ]; then
    echo "Error: Amiga payload overlaps the kickety split area" >&2
    exit 1
  fi

  fill_range_with_random_bytes "${file_path}" "${payload_end_bytes}" \
    "$((AMIGA_KICKETY_OFFSET_BYTES - payload_end_bytes))"
  fill_range_with_random_bytes "${file_path}" "${kickety_end_bytes}" \
    "$((CRC_FIELD_OFFSET_BYTES - kickety_end_bytes))"
  python3 - "${file_path}" "${CRC_FIELD_OFFSET_BYTES}" <<'PY'
import sys

path = sys.argv[1]
check_offset = int(sys.argv[2])

with open(path, "r+b") as handle:
    handle.seek(check_offset)
    handle.write(b"\x00\x00\x00\x00")
PY
}

publish_dist_artifacts() {
  dist_dir="dist/${TARGET_PLATFORM}"
  mkdir -p "${dist_dir}"

  if [ "${TARGET_PLATFORM}" = "amiga" ]; then
    rm -f "${dist_dir}/${DIST_ROM_IMAGE_NAME}" \
      "${dist_dir}"/RESCUE_SWITCHER_v*_512KB_*.img \
      "${dist_dir}"/RESCUE_SWITCHER_vv*.img
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
    rm -f "build/amiga/${SOURCE_ROM_IMAGE_NAME}" build/amiga/ROMSWAMI.MAP \
      build/amiga/RESCUE_SWITCHER_vv*.img
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
    finalize_st_rom_image "build/st/${SOURCE_ROM_IMAGE_NAME}"
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

  finalize_amiga_rom_image "build/amiga/${SOURCE_ROM_IMAGE_NAME}" \
    "build/amiga/ROMSWAMI.MAP"
  romtool copy -c "build/amiga/${SOURCE_ROM_IMAGE_NAME}" \
    "build/amiga/${SOURCE_ROM_IMAGE_NAME}.tmp"
  mv "build/amiga/${SOURCE_ROM_IMAGE_NAME}.tmp" "build/amiga/${SOURCE_ROM_IMAGE_NAME}"
  write_check_field "build/amiga/${SOURCE_ROM_IMAGE_NAME}" \
    "${TARGET_ROM_IMAGE_SIZE_BYTES}" "${CRC_FIELD_OFFSET_BYTES}" \
    "${SECOND_IGNORED_FIELD_OFFSET_BYTES}" "${SECOND_IGNORED_FIELD_SIZE_BYTES}"
  romtool copy -c "build/amiga/${SOURCE_ROM_IMAGE_NAME}" \
    "build/amiga/${SOURCE_ROM_IMAGE_NAME}.tmp"
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
  if [ -f "build/st/${SOURCE_ROM_IMAGE_NAME}" ]; then
    finalize_st_rom_image "build/st/${SOURCE_ROM_IMAGE_NAME}"
  fi
fi
if [ "${TEST_MODE}" = "0" ]; then
  publish_dist_artifacts
fi
