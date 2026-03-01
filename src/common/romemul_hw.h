#pragma once

#include <stdint.h>

/**
 * Hardware abstraction for the SidecarTridge ROM Emulator control block.
 *
 * The actual register map depends on the target installation. Update the
 * addresses below to match the CPLD/MCU interface exposed by the emulator.
 */

typedef struct {
  volatile uint8_t bank_select;  ///< Write bank index (0-255)
  volatile uint8_t command;      ///< Command register (latch / unlock etc.)
  volatile uint8_t status;       ///< Status bits (busy, error, current bank)
  volatile uint8_t reserved;
} romemul_ctrl_t;

#define ROMEMUL_CTRL_BASE ((uintptr_t)0x00FF8000)  // placeholder address

#define ROMEMUL_CTRL ((romemul_ctrl_t *)ROMEMUL_CTRL_BASE)

#define ROMEMUL_CMD_LATCH (1U << 0)
#define ROMEMUL_CMD_UNLOCK (1U << 1)

#define ROMEMUL_STATUS_BUSY (1U << 0)
#define ROMEMUL_STATUS_ERR (1U << 1)

static inline void romemul_select_bank(uint8_t bank) {
  ROMEMUL_CTRL->bank_select = bank;
  ROMEMUL_CTRL->command = ROMEMUL_CMD_LATCH;
}

static inline void romemul_wait_ready(void) {
  while (ROMEMUL_CTRL->status & ROMEMUL_STATUS_BUSY) {
    __asm__ volatile("nop\n nop\n" ::: "memory");
  }
}
