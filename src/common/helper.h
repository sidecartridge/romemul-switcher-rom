#ifndef HELPER_H_
#define HELPER_H_

typedef void (*helper_trace_fn_t)(const char *message);

void init_rom_address(unsigned long rom_base_address,
                      helper_trace_fn_t trace_fn);
void send_change_rom_command_and_hard_reset(unsigned char rom_index);

int read_flash_block(void *memory_exchange, unsigned long flash_address_offset,
                     unsigned short block_size);
int read_flash_page(void *memory_exchange, unsigned long flash_address_offset,
                    unsigned char endian);

#endif /* HELPER_H_ */
