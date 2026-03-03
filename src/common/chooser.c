#include "chooser.h"

#include "palloc.h"
#include "test.h"
#include "text.h"
#include "commands.h"
#include "../st/kbd.h"

enum {
  kScreenWidthChars = 80,
  kPaginatedContentYOffset = 5,
  kMetadataFlagActive = 0x1U,
  kMetadataFlagRescue = 0x2U,
  kSwitcherTosProtocolVersion = 0x0031
};

#define ENDIAN_BIG 1
#define FLASH_CATALOG_START 0xFF0000UL
#define FLASH_PARAMS_START 0xFFF000UL
#define READ_ROM_PAGE_SIZE 4096UL
#define MEMORY_EXCHANGE_SIZE 61440UL
#define MEMORY_PARAMS_EXCHANGE_SIZE 4096UL

#define BIG_ENDIAN_TO_LITTLE_ENDIAN_WORD(x) \
  (((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8)

static void copy_name(char dst[ROM_NAME_SIZE], const unsigned char *src) {
  unsigned int i = 0U;
  while (i + 1U < ROM_NAME_SIZE && src[i] != 0U) {
    dst[i] = (char)src[i];
    ++i;
  }
  dst[i] = '\0';
}

int parse_rom_description(unsigned char *rom_desc_raw, unsigned char *rom_params_raw,
                          rom_catalog_t **rom_desc) {
  unsigned long default_rom_index =
      (unsigned long)rom_params_raw[DEFAULT_ROM_INDEX] |
      ((unsigned long)rom_params_raw[DEFAULT_ROM_INDEX + 1] << 8);
  unsigned long rescue_rom_index =
      (unsigned long)rom_params_raw[RESCUE_ROM_INDEX] |
      ((unsigned long)rom_params_raw[RESCUE_ROM_INDEX + 1] << 8);

#if defined(_DEBUG) && (_DEBUG > 0)
  text_printf("Default ROM index: %lu\n\r", default_rom_index);
  text_printf("Rescue ROM index: %lu\n\r", rescue_rom_index);
#endif

  int count = 0;
  int scan_offset = 0;
  while (rom_desc_raw[scan_offset] != 0U) {
    count++;
    scan_offset += ROM_DESCRIPTION_SIZE;
  }

  if (count <= 0) {
    *rom_desc = (rom_catalog_t *)0;
    return 0;
  }

  if ((unsigned long)count > (~0UL / (unsigned long)sizeof(rom_catalog_t))) {
    *rom_desc = (rom_catalog_t *)0;
    return 0;
  }

  *rom_desc = (rom_catalog_t *)pa_alloc(((unsigned long)count) << 8);
  if (*rom_desc == (rom_catalog_t *)0) {
    return 0;
  }

  int offset = 0;
  for (int i = 0; i < count; i++, offset += ROM_DESCRIPTION_SIZE) {
    copy_name((*rom_desc)[i].name, &rom_desc_raw[offset]);
    (*rom_desc)[i].blocks =
        ((unsigned long)rom_desc_raw[offset + ROM_NAME_SIZE] << 8) |
        (unsigned long)rom_desc_raw[offset + ROM_NAME_SIZE + 1];

    unsigned short metadata = 0;
    if (i == (int)default_rom_index) {
      metadata = kMetadataFlagActive;
    }
    if (i == (int)rescue_rom_index) {
      metadata = (unsigned short)(metadata | kMetadataFlagRescue);
    }
    (*rom_desc)[i].metadata = metadata;
  }

  return count;
}

char *create_file_array(rom_catalog_t *rom_desc, int num_entries) {
  int total_length = 0;
  for (int i = 0; i < num_entries; i++) {
    total_length += kScreenWidthChars + 1;
  }

  char *file_string = (char *)pa_alloc((unsigned long)(total_length + 1));
  if (file_string == (char *)0) {
    return (char *)0;
  }

  const int custom_memory_info_size = 11;
  const int custom_rom_name_size = kScreenWidthChars - custom_memory_info_size;

  char *current_ptr = file_string;
  for (int i = 0; i < num_entries; i++) {
    int trimmed_str_len = 0;
    while (trimmed_str_len < custom_rom_name_size &&
           trimmed_str_len < ROM_NAME_SIZE &&
           rom_desc[i].name[trimmed_str_len] != '\0') {
      trimmed_str_len++;
    }

    if (trimmed_str_len > 0) {
      for (int j = 0; j < trimmed_str_len; ++j) {
        current_ptr[j] = rom_desc[i].name[j];
      }
    }
    for (int j = trimmed_str_len; j < custom_rom_name_size; j++) {
      current_ptr[j] = ' ';
    }
    current_ptr += custom_rom_name_size;

    unsigned long kb_val =
        ((unsigned long)BIG_ENDIAN_TO_LITTLE_ENDIAN_WORD(rom_desc[i].blocks))
        << 2;
    if (kb_val > 999UL) {
      kb_val = 999UL;
    }

    current_ptr[0] = ' ';
    unsigned long rem = kb_val;
    unsigned long d1v = 0UL;
    unsigned long d2v = 0UL;
    unsigned long d3v;
    while (rem >= 100UL) {
      rem -= 100UL;
      d1v++;
    }
    while (rem >= 10UL) {
      rem -= 10UL;
      d2v++;
    }
    d3v = rem;
    current_ptr[1] = (d1v > 0UL) ? (char)('0' + (char)d1v) : ' ';
    current_ptr[2] =
        ((d1v > 0UL) || (d2v > 0UL)) ? (char)('0' + (char)d2v) : ' ';
    current_ptr[3] = (char)('0' + (char)d3v);
    current_ptr[4] = ' ';
    current_ptr[5] = 'K';
    current_ptr[6] = 'B';
    current_ptr[7] = ' ';
    current_ptr[8] = ((rom_desc[i].metadata & kMetadataFlagActive) != 0U)
                         ? 'A'
                         : ' ';
    current_ptr[9] = ((rom_desc[i].metadata & kMetadataFlagRescue) != 0U)
                         ? 'R'
                         : ' ';
    current_ptr[10] = ' ';

    current_ptr += custom_memory_info_size;
    *current_ptr = '\0';
    current_ptr++;
  }

  file_string[total_length] = '\0';
  return file_string;
}

static char *print_file_at_index(char *current_ptr, unsigned short index) {
  unsigned short current_index = 0;
  while (*current_ptr) {
    if (current_index == index) {
      deleteLineFromCursor();
      text_printf(current_ptr);
      while (*current_ptr) {
        current_ptr++;
      }
      return current_ptr;
    }

    while (*current_ptr) {
      current_ptr++;
    }
    current_ptr++;
    current_index++;
  }

  text_printf("No file found at index %u\r\n", index);
  return current_ptr;
}

static void highlight_and_print(char *file_array, unsigned short index,
                                unsigned short offset, int current_line,
                                unsigned short highlight) {
  text_set_cursor(0, current_line + 2 + index - offset);
  if (highlight) {
    invertVideoEnable();
  }
  deleteLineFromCursor();
  print_file_at_index(file_array, index);
  if (highlight) {
    invertVideoDisable();
  }
}

static __attribute__((noinline)) int mul_u16(int a, int b) {
  unsigned short ua;
  unsigned short ub;
  unsigned short out = 0U;

  if (a <= 0 || b <= 0) {
    return 0;
  }

  ua = (unsigned short)a;
  ub = (unsigned short)b;
  while (ub != 0U) {
    if ((ub & 1U) != 0U) {
      out = (unsigned short)(out + ua);
    }
    ua = (unsigned short)(ua << 1);
    ub = (unsigned short)(ub >> 1);
  }

  return (int)out;
}

static int div_u16(int n, int d) {
  int q = 0;
  if (n <= 0 || d <= 0) {
    return 0;
  }
  while (n >= d) {
    n -= d;
    q++;
  }
  return q;
}

int display_paginated_content(char *file_array, int num_files, int page_size,
                              char *item_name, char *extra_bar,
                              unsigned long *keypress) {
  int selected_rom = -1;
  int page_number = 0;
  int current_index = 0;
  text_set_cursor(0, page_size + kPaginatedContentYOffset);
  text_printf("[UP] and [DOWN] to select. [LEFT] and [RIGHT] to paginate.\r\n");
  text_printf(extra_bar);

  while (selected_rom < 0) {
    int start_index = mul_u16(page_size, page_number);
    int end_index = start_index + page_size - 1;
    int max_page = 0;
    int page_end_index = start_index + page_size;

    if (num_files > 1 && page_size > 0) {
      max_page = div_u16(num_files - 1, page_size);
    }

    if (start_index >= num_files) {
      text_printf("No content for this page number!\r\n");
      return -1;
    }

    if (end_index >= num_files) {
      end_index = num_files - 1;
    }

    char *current_ptr = file_array;
    int index = 0;
    int current_line = 2 + (ELEMENTS_PER_PAGE - page_size);
    text_set_cursor(0, current_line);
    deleteLineFromCursor();
    text_printf("%s found: %d. ", item_name, num_files);
    text_printf("Page %d of %d\r\n", page_number + 1, max_page + 1);
    text_set_cursor(0, current_line + 1);
    deleteLineFromCursor();
    while (index <= num_files) {
      if ((start_index <= index) && (index <= end_index)) {
        text_set_cursor(0, current_line + 2 + index - start_index);
        current_ptr = print_file_at_index(file_array, (unsigned short)index);
        index++;
      } else {
        while (*current_ptr != 0x00) {
          current_ptr++;
        }
        current_ptr++;
        index++;
      }
    }
    for (int i = index; i <= page_end_index; i++) {
      text_set_cursor(0, -1 + current_line + 2 + i - start_index);
      deleteLineFromCursor();
    }

    unsigned char key;
    unsigned short change_page = 0U;
    while ((selected_rom < 0) && (!change_page)) {
      highlight_and_print(file_array, (unsigned short)current_index,
                          (unsigned short)start_index, current_line, 1U);
      key = kbd_poll_scancode_wait();
      if (keypress != (unsigned long *)0) {
        *keypress = key;
      }
      switch (key) {
        case KEY_UP_ARROW:
          if (current_index > start_index) {
            highlight_and_print(file_array, (unsigned short)current_index,
                                (unsigned short)start_index, current_line, 0U);
            current_index = current_index - 1;
          }
          break;

        case KEY_DOWN_ARROW:
          if (current_index < end_index) {
            highlight_and_print(file_array, (unsigned short)current_index,
                                (unsigned short)start_index, current_line, 0U);
            current_index = current_index + 1;
          }
          break;
        case KEY_LEFT_ARROW:
          if (page_number > 0) {
            page_number--;
            current_index = mul_u16(page_size, page_number);
            change_page = 1U;
          }
          break;
        case KEY_RIGHT_ARROW:
          if (page_number < max_page) {
            page_number++;
            current_index = mul_u16(page_size, page_number);
            change_page = 1U;
          }
          break;
#if defined(KEY_ENTER) && defined(KEY_RETURN) && (KEY_ENTER != KEY_RETURN)
        case KEY_ENTER:
#endif
        case KEY_RETURN:
          selected_rom = current_index + 1;
          return selected_rom;
        case KEY_ESC:
          return -1;
        case KEY_D:
        case KEY_R:
          return current_index + 1;
        case KEY_M:
          return -1;
        case KEY_U:
          return -1;
        default:
          if (keypress != (unsigned long *)0) {
            *keypress = key;
          }
          break;
      }

#if defined(_DEBUG) && (_DEBUG > 0)
      text_set_cursor(0, kPaginatedContentYOffset + page_size);
      text_printf("Key: 0x%02x    Index: %d", (unsigned int)key, current_index);
#endif
    }
  }
  return -1;
}

void chooser_loop(unsigned long rom_base_addr, helper_trace_fn_t trace_fn,
                  unsigned char default_color, const char *computer_model) {
  char titleLine[SCR_WIDTH_CHARS + 1U];

  text_printf("Loading available ROM images...");
  text_printf("\r\n");

  unsigned char *buffer =
      (unsigned char *)pa_alloc_aligned(MEMORY_EXCHANGE_SIZE, 16UL);
  if (!buffer) {
    text_printf(
        "OOM error allocating memory for the ROM buffer %lu-byte aligned "
        "buffer.\r\n",
        (unsigned long)MEMORY_EXCHANGE_SIZE);
    return;
  }

  unsigned char *buffer_params =
      (unsigned char *)pa_alloc_aligned(MEMORY_PARAMS_EXCHANGE_SIZE, 16UL);
  if (!buffer_params) {
    text_printf(
        "OOM error allocating memory for the ROM parameters buffer %lu-byte "
        "aligned buffer.\r\n",
        (unsigned long)MEMORY_PARAMS_EXCHANGE_SIZE);
    return;
  }

  rom_catalog_t *rom_descriptions;
  int num_entries = 0;
  int flow_control = 0;
  int rom_number = 0;
  signed short protocol_version = 0;

  init_rom_address(rom_base_addr, trace_fn);

#if (defined(_TEST) && (_TEST > 0))
  num_entries =
      parse_rom_description(flashCatalogRaw, flashParamsRaw, &rom_descriptions);
  text_printf("Number of ROM entries: %d\n\r", num_entries);
  protocol_version =
      (signed short)((unsigned short)flashParamsRaw[ROM_DESCRIPTION_SIZE * 2] |
                     ((unsigned short)
                          flashParamsRaw[1 + (ROM_DESCRIPTION_SIZE * 2)]
                      << 8));
  text_printf("Protocol version: %x\n\r", protocol_version);
#else
  int err = read_flash_page(buffer_params, FLASH_PARAMS_START, ENDIAN_BIG);
  if (err != 0) {
    text_printf("Error reading the flash page at address %08lx. Exiting.\r\n",
                (unsigned long)FLASH_PARAMS_START);
    kbd_wait_for_key_press();
    return;
  }

  const unsigned long pages = MEMORY_EXCHANGE_SIZE / READ_ROM_PAGE_SIZE;
  for (unsigned long i = 0U; i < pages; i++) {
    unsigned long addr =
        (unsigned long)(FLASH_CATALOG_START +
                        (unsigned long)i * (unsigned long)READ_ROM_PAGE_SIZE);
    err = read_flash_page(buffer + i * READ_ROM_PAGE_SIZE, addr, ENDIAN_BIG);
    if (err != 0) {
      text_printf("Error reading the flash page at address %08lx. Exiting.\r\n",
                  (unsigned long)addr);
      kbd_wait_for_key_press();
      return;
    }
  }

  protocol_version = (signed short)(buffer_params[ROM_DESCRIPTION_SIZE * 2] +
                                    (buffer_params[1 + (ROM_DESCRIPTION_SIZE * 2)] *
                                     256U));

  num_entries = parse_rom_description(buffer, buffer_params, &rom_descriptions);
#endif

  char *file_array = create_file_array(rom_descriptions, num_entries);

  if (protocol_version != kSwitcherTosProtocolVersion) {
    text_printf(
        "This version of SWITCHER tool is not compatible with the firmware of "
        "the device.\r\n");
    text_printf("Press any key to exit...\r\n");
    kbd_wait_for_key_press();
    return;
  }

  while (flow_control == 0) {
    text_clear();
    text_set_color(default_color);
    text_build_title_line(titleLine, computer_model);

    text_printf("\x1BY  \x1Bp%s\x1Bq", titleLine);
    text_printf("\x1BY\" ");

    if (num_entries == 0) {
      text_printf(
          "No ROMs found. Do you have a Sidecartrige device configured?\r\n");
      text_printf("Press any key to exit...\r\n");
      kbd_wait_for_key_press();
      return;
    } else {
      unsigned long keypress = 0U;
      rom_number = display_paginated_content(
          file_array, num_entries, ELEMENTS_PER_PAGE, "ROM images",
          "[ENTER] or [RETURN] to load the ROM.", &keypress);
    }

    if (rom_number > 0) {
      unsigned char confirm_key;

      text_set_cursor(0, 22);
      deleteLineFromCursor();
      text_printf("ROM image selected: %s\n",
                  rom_descriptions[rom_number - 1].name);
      deleteLineFromCursor();
      text_printf(
          "Press any key to load the ROM image and reset the computer (ESC to "
          "cancel).\r");

      confirm_key = kbd_poll_scancode_wait();
      if (confirm_key == KEY_ESC) {
        rom_number = 0;
        continue;
      }

      flow_control = 1;
    }
  }

  deleteLineFromCursor();
  text_printf("Rebooting the computer with the new ROM...");
  send_change_rom_command_and_hard_reset((unsigned char)(rom_number - 1));
}
