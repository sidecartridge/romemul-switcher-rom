DEBUG ?= 0
TEST ?= 0
ROM_BASE_ADDR_UL ?= 0x00E00000UL

ifneq ($(strip $(LIBS)$(LDLIBS)),)
$(error External libraries are forbidden: LIBS/LDLIBS must be empty)
endif

ifeq ($(DEBUG),1)
ifeq ($(TEST),1)
BUILD_DIR := ../../build/st-debug-test
else
BUILD_DIR := ../../build/st-debug
endif
TARGET_BASE := ROMSWDBG
OPT_FLAGS := -O0 -g
DEBUG_DEFS := -D_DEBUG=1
else
ifeq ($(TEST),1)
BUILD_DIR := ../../build/st-test
else
BUILD_DIR := ../../build/st
endif
TARGET_BASE := ROMSWITC
OPT_FLAGS := -O2
DEBUG_DEFS :=
endif

ifeq ($(TEST),1)
TEST_DEFS := -D_TEST=1 -DTEST=1
else
TEST_DEFS :=
endif

TARGET_PRG := $(BUILD_DIR)/$(TARGET_BASE).PRG
TARGET_BIN := $(BUILD_DIR)/$(TARGET_BASE).BIN
TARGET_MAP := $(BUILD_DIR)/$(TARGET_BASE).MAP

CC := m68k-atari-mint-gcc
AS := m68k-atari-mint-gcc
OBJCOPY := m68k-atari-mint-objcopy

APP_VERSION := $(strip $(shell if [ -f ../../version.txt ]; then tr -d '\r\n' < ../../version.txt; else echo 0.0.0; fi))
NO_EXT_LIB_FLAGS := -nostdlib -nodefaultlibs -nostartfiles

CFLAGS := -m68000 -std=gnu99 -ffreestanding -fomit-frame-pointer $(OPT_FLAGS) -Wall -Wextra -Werror $(NO_EXT_LIB_FLAGS) $(DEBUG_DEFS) $(TEST_DEFS) -DAPP_VERSION_STR=\"$(APP_VERSION)\" -DROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL)
ASFLAGS := -m68000 -x assembler-with-cpp $(NO_EXT_LIB_FLAGS) $(DEBUG_DEFS) $(TEST_DEFS)
LDFLAGS := $(NO_EXT_LIB_FLAGS) -Wl,--traditional-format,-Map=$(TARGET_MAP),--entry,_start

SRCS_C := rom_switcher.c text.c screen.c kbd.c glyph.c hatari_trace.c portable_alloc.c helper.c
ifneq ($(filter 1,$(DEBUG) $(TEST)),)
SRCS_C += test.c
endif
SRCS_S := start.s natfeat.s
VPATH := . ../common

# start.s must be linked first so PRG execution begins at _start.
OBJS := $(SRCS_S:%=$(BUILD_DIR)/%.o) $(SRCS_C:%=$(BUILD_DIR)/%.o)

all: $(TARGET_PRG) $(TARGET_BIN)

$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.s.o: %.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/helper.c.o: ../common/helper.h

$(TARGET_PRG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(TARGET_BIN): $(TARGET_PRG)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf ../../build/st ../../build/st-test ../../build/st-debug \
	  ../../build/st-debug-test

.PHONY: all clean
