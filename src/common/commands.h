/**
 * File: src/common/commands.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared ROM command declarations.
 */

#ifndef HELPER_H_
#define HELPER_H_

/* Where the catalog and the parameters page live in the device's flash,
   relative to XIP_BASE: the firmware's layout (C-04, D-03). */
#define FLASH_CATALOG_START 0xFF0000UL
#define FLASH_PARAMS_START 0xFFF000UL

typedef void (*helper_trace_fn_t)(const char *message);

void init_rom_address(unsigned long rom_base_address,
                      helper_trace_fn_t trace_fn);
void send_change_rom_command_and_hard_reset(unsigned char rom_index);

/* One CMD_PING, then the image's first long restored at base+0 (EPIC-03).
   0: answered; -1: no answer; -2: checksum error (0xFFFFFFFD); -3: answered,
   but the restore did not take. */
int command_ping(void);

int read_flash_page(void *memory_exchange, unsigned long flash_address_offset,
                    unsigned char endian);

/* Block reads repeated so far because a command failed or a block's checksum
   did not match (EPIC-03 STORY-07). */
unsigned long command_block_retries(void);

#endif /* HELPER_H_ */
