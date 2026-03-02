#!/bin/sh
set -eu

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

# In release mode, remove potentially copied debug/test artifacts first.
# Otherwise `make` may think build/st/ROMSWITC.* is up-to-date and skip rebuild.
if [ "${DEBUG_MODE}" = "0" ] && [ "${TEST_MODE}" = "0" ]; then
  rm -f build/st/ROMSWITC.PRG build/st/ROMSWITC.BIN build/st/ROMSWITC.MAP
fi

STCMD_NO_TTY=1 ST_WORKING_FOLDER=/Users/diego/mister_wkspc/romemul-switcher-rom stcmd make st DEBUG="${DEBUG_MODE}" TEST="${TEST_MODE}"

# Publish the selected build artifact to build/st/ROMSWITC.* so runtime tools
# that always load that path execute the chosen mode (release/debug/test).
if [ "${DEBUG_MODE}" = "1" ] && [ "${TEST_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-debug-test/ROMSWDBG.PRG build/st/ROMSWITC.PRG
  cp build/st-debug-test/ROMSWDBG.BIN build/st/ROMSWITC.BIN
  cp build/st-debug-test/ROMSWDBG.MAP build/st/ROMSWITC.MAP
elif [ "${DEBUG_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-debug/ROMSWDBG.PRG build/st/ROMSWITC.PRG
  cp build/st-debug/ROMSWDBG.BIN build/st/ROMSWITC.BIN
  cp build/st-debug/ROMSWDBG.MAP build/st/ROMSWITC.MAP
elif [ "${TEST_MODE}" = "1" ]; then
  mkdir -p build/st
  cp build/st-test/ROMSWITC.PRG build/st/ROMSWITC.PRG
  cp build/st-test/ROMSWITC.BIN build/st/ROMSWITC.BIN
  cp build/st-test/ROMSWITC.MAP build/st/ROMSWITC.MAP
fi
