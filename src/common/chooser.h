#ifndef CHOOSER_H_
#define CHOOSER_H_

#include "commands.h"

#define ROM_NAME_SIZE 64
#define ROM_DESCRIPTION_SIZE 256
#define ROM_COMPRESSED_CLUSTER_SIZE 184
#define DEFAULT_ROM_INDEX 70
#define RESCUE_ROM_INDEX (256 + DEFAULT_ROM_INDEX)
#define ELEMENTS_PER_PAGE 17

typedef struct {
  char name[ROM_NAME_SIZE];
  unsigned long blocks;
  unsigned long metadata;
  unsigned char compressed_blocks[ROM_COMPRESSED_CLUSTER_SIZE];
} rom_catalog_t;

int parse_rom_description(unsigned char *rom_desc_raw, unsigned char *rom_params_raw,
                          rom_catalog_t **rom_desc);
char *create_file_array(rom_catalog_t *rom_desc, int num_entries);
int display_paginated_content(char *file_array, int num_files, int page_size,
                              char *item_name, char *extra_bar,
                              unsigned long *keypress);
void chooser_loop(unsigned long rom_base_addr, helper_trace_fn_t trace_fn,
                  unsigned char default_color, const char *computer_model);

#endif /* CHOOSER_H_ */
