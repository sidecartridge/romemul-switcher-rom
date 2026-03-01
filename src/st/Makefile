BUILD_DIR := ../../build/st
TARGET_PRG := $(BUILD_DIR)/romemul_switcher.prg
TARGET_BIN := $(BUILD_DIR)/romemul_switcher.bin
TARGET_MAP := $(BUILD_DIR)/romemul_switcher.map

CC := m68k-atari-mint-gcc
AS := m68k-atari-mint-gcc
OBJCOPY := m68k-atari-mint-objcopy

CFLAGS := -m68000 -std=gnu99 -ffreestanding -fomit-frame-pointer -O2 -Wall -Wextra -Werror -nostdlib
ASFLAGS := -m68000 -x assembler-with-cpp -nostdlib
LDFLAGS := -nostdlib -Wl,--traditional-format,-Map=$(TARGET_MAP)

SRCS_C := rom_switcher.c
SRCS_S := start.s

OBJS := $(SRCS_C:%=$(BUILD_DIR)/%.o) $(SRCS_S:%=$(BUILD_DIR)/%.o)

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
