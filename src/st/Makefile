DEBUG ?= 0
TEST ?= 0
STARTUP_ROM_ASM ?= startup_ste.s
STARTUP_ROM_OBJ := $(STARTUP_ROM_ASM).o
ROM_BASE_ADDR_LD := $(patsubst %UL,%,$(patsubst %ul,%,$(ROM_BASE_ADDR_UL)))

ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifndef ROM_BASE_ADDR_UL
$(error ROM_BASE_ADDR_UL is required. Pass ROM_BASE_ADDR_UL=0x00E00000UL from the calling script)
endif
endif

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
TARGET_ROM_MAP := $(BUILD_DIR)/ROMSWROM.MAP
ROM_LDSCRIPT := $(BUILD_DIR)/rom_abs.ld
BUILD_CONFIG_STAMP := $(BUILD_DIR)/.build_config

CC := m68k-atari-mint-gcc
AS := m68k-atari-mint-gcc
OBJCOPY := m68k-atari-mint-objcopy

APP_VERSION := $(strip $(shell if [ -f ../../version.txt ]; then tr -d '\r\n' < ../../version.txt; else echo 0.0.0; fi))
ROM_IMAGE_NAME := RESCUE_SWITCHER_v$(APP_VERSION).img
TARGET_ROM_BIN := $(BUILD_DIR)/$(ROM_IMAGE_NAME)
NO_EXT_LIB_FLAGS := -nostdlib -nodefaultlibs -nostartfiles

CFLAGS := -m68000 -std=gnu99 -ffreestanding -fomit-frame-pointer $(OPT_FLAGS) -Wall -Wextra -Werror $(NO_EXT_LIB_FLAGS) $(DEBUG_DEFS) $(TEST_DEFS) -DAPP_VERSION_STR=\"$(APP_VERSION)\" -DROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL)
ASFLAGS_BASE := -m68000 -x assembler-with-cpp $(NO_EXT_LIB_FLAGS) $(DEBUG_DEFS) $(TEST_DEFS)
ASFLAGS := $(ASFLAGS_BASE) -DROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL)
LDFLAGS := $(NO_EXT_LIB_FLAGS) -Wl,--traditional-format,-Map=$(TARGET_MAP),--entry,_start
LDFLAGS_ROM := $(NO_EXT_LIB_FLAGS) -Wl,-Map=$(TARGET_ROM_MAP),--entry,_os_entry,-T,$(ROM_LDSCRIPT),--oformat,binary

SRCS_C := switcher.c text.c screen.c kbd.c glyph.c platform.c htrace.c palloc.c commands.c chooser.c
ifeq ($(TEST),1)
SRCS_C += test.c
endif
SRCS_S_PRG := start.s natfeat.s
SRCS_S_ROM := $(STARTUP_ROM_ASM) natfeat.s
VPATH := . ../common

# start.s must be linked first so PRG execution begins at _start.
OBJS_C := $(SRCS_C:%=$(BUILD_DIR)/%.o)
OBJS_PRG := $(SRCS_S_PRG:%=$(BUILD_DIR)/%.o) $(OBJS_C)
OBJS_ROM := $(SRCS_S_ROM:%=$(BUILD_DIR)/%.o) $(OBJS_C)

ifeq ($(TEST),1)
ALL_TARGETS := $(TARGET_PRG) $(TARGET_BIN)
else
ALL_TARGETS := $(TARGET_PRG) $(TARGET_BIN) $(TARGET_ROM_BIN)
endif

all: $(ALL_TARGETS)
rom: $(TARGET_ROM_BIN)

.PHONY: FORCE
FORCE:

$(BUILD_CONFIG_STAMP): FORCE | $(BUILD_DIR)
	@printf '%s\n' "ROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL) STARTUP_ROM_ASM=$(STARTUP_ROM_ASM) DEBUG=$(DEBUG) TEST=$(TEST)" > $@.tmp
	@if [ ! -f $@ ] || ! cmp -s $@.tmp $@; then mv $@.tmp $@; else rm -f $@.tmp; fi

$(BUILD_DIR)/%.c.o: %.c $(BUILD_CONFIG_STAMP) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.s.o: %.s $(BUILD_CONFIG_STAMP) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(ROM_LDSCRIPT): rom_abs.ld.S mem.h Makefile $(BUILD_CONFIG_STAMP) | $(BUILD_DIR)
	$(CC) -E -P -x c -D__ASSEMBLER__=1 -DROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_LD) \
	  -DSTARTUP_ROM_OBJ=$(STARTUP_ROM_OBJ) $< -o $@.tmp
	mv $@.tmp $@

$(BUILD_DIR)/commands.c.o: ../common/commands.h
$(BUILD_DIR)/chooser.c.o: ../common/chooser.h

$(TARGET_PRG): $(OBJS_PRG)
	$(CC) $(LDFLAGS) -o $@ $(OBJS_PRG)

$(TARGET_BIN): $(TARGET_PRG)
	$(OBJCOPY) -O binary $< $@

$(TARGET_ROM_BIN): $(OBJS_ROM) $(ROM_LDSCRIPT)
	$(CC) $(LDFLAGS_ROM) -o $@ $(OBJS_ROM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf ../../build/st ../../build/st-test ../../build/st-debug \
	  ../../build/st-debug-test

.PHONY: all clean rom
