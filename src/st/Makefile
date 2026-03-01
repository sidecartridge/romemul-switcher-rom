BUILD_DIR := ../../build/st
TARGET_BASE := ROMSWITC
TARGET_PRG := $(BUILD_DIR)/$(TARGET_BASE).PRG
TARGET_BIN := $(BUILD_DIR)/$(TARGET_BASE).BIN
TARGET_MAP := $(BUILD_DIR)/$(TARGET_BASE).MAP

CC := m68k-atari-mint-gcc
AS := m68k-atari-mint-gcc
OBJCOPY := m68k-atari-mint-objcopy

APP_VERSION := $(strip $(shell if [ -f ../../version.txt ]; then tr -d '\r\n' < ../../version.txt; else echo 0.0.0; fi))

CFLAGS := -m68000 -std=gnu99 -ffreestanding -fomit-frame-pointer -O2 -Wall -Wextra -Werror -nostdlib -DAPP_VERSION_STR=\"$(APP_VERSION)\"
ASFLAGS := -m68000 -x assembler-with-cpp -nostdlib
LDFLAGS := -nostdlib -Wl,--traditional-format,-Map=$(TARGET_MAP),--entry,_start

SRCS_C := rom_switcher.c text.c screen.c kbd.c glyph.c
SRCS_S := start.s
VPATH := . ../common

# start.s must be linked first so PRG execution begins at _start.
OBJS := $(SRCS_S:%=$(BUILD_DIR)/%.o) $(SRCS_C:%=$(BUILD_DIR)/%.o)

all: $(TARGET_PRG) $(TARGET_BIN)

$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.s.o: %.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(TARGET_PRG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(TARGET_BIN): $(TARGET_PRG)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
