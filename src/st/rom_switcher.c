#include "rom_switcher.h"

#include "../common/helper.h"
#include "../common/portable_alloc.h"
#include "../common/test.h"
#include "../common/text.h"
#if defined(_DEBUG) && (_DEBUG > 0)
#include "hatari_trace.h"
#endif
#include "kbd.h"
#include "screen.h"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif

#ifndef ROM_BASE_ADDR_UL
#define ROM_BASE_ADDR_UL 0x00E00000UL
#endif

enum {
  kInvalidScancode = 0xFFU,
  kKeyReleaseMask = 0x80U,
  kPollDelayLoops = 2500U,
  kStatusRegisterIplMask = 0x0700U,
  kIplLevelMask = 0x7U,
  kIplShiftBits = 8U,
  kTitleWidthChars = 80U,
  kTitleBufferSize = 81U,
  kPolledInterruptLevel = 7U,
  kScancodeKey1 = 0x02U,
  kScancodeKey2 = 0x03U,
  kScancodeKey3 = 0x04U,
  kScancodeKey4 = 0x05U,
  kHeapReservedBytes = 8U,
  kColorDefault = 1U,
  kColorActive = 2U,
  kColorPrompt = 3U
};

#define ENDIAN_BIG 1
#define ENDIAN_LITTLE 0

#define FLASH_STORAGE_START 0x040000
#define FLASH_STORAGE_LENGTH 16777216
#define FLASH_CATALOG_START 0xFF0000
#define FLASH_PARAMS_START 0xFFF000

#define READ_ROM_PAGE_SIZE 4096  // bytes to read from the storage memory

#define ROM_NAME_SIZE 64
#define ROM_MAX_FILES 64
#define ROM_DESCRIPTION_SIZE 256
#define ROM_COMPRESSED_CLUSTER_SIZE 184
#define DEFAULT_ROM_INDEX 70
#define RESCUE_ROM_INDEX (256 + DEFAULT_ROM_INDEX)

#define SCREEN_H_WIDTH 80
#define ELEMENTS_PER_PAGE 17
#define PAGINATED_CONTENT_Y_OFFSET 5

// VERSION
#define SWITCHER_TOS_PROTOCOL_VERSION 0x0031

/* IKBD make scancodes (8-bit). */
#define KEY_UP_ARROW 0x48U
#define KEY_DOWN_ARROW 0x50U
#define KEY_LEFT_ARROW 0x4BU
#define KEY_RIGHT_ARROW 0x4DU

#define KEY_RETURN 0x1CU
#define KEY_ENTER 0x72U
#define KEY_ESC 0x01U
#define KEY_D 0x20U
#define KEY_M 0x32U
#define KEY_U 0x16U
#define KEY_R 0x13U
#define KEY_Y 0x1AU

#define invertVideoEnable() text_printf("\033p")   // Enable invert video mode
#define invertVideoDisable() text_printf("\033q")  // Disable invert video mode
#define deleteLineFromCursor() \
  text_printf("\033K")                     // Delete from cursor to end of line
#define deleteLine() text_printf("\033M")  // Delete

typedef struct {
  char name[ROM_NAME_SIZE];  // 64 bytes
  unsigned long blocks;      // 4 bytes
  unsigned long metadata;    // 4 bytes
  unsigned char
      compressed_blocks[ROM_COMPRESSED_CLUSTER_SIZE];  // 184 bytes (to
                                                       // make total size
                                                       // 256 bytes)
} rom_catalog_t;

#define BIG_ENDIAN_TO_LITTLE_ENDIAN_WORD(x) \
  (((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8)

static void setInterruptLevelMask(unsigned short maskLevel) {
  unsigned short statusRegister;
  __asm__ volatile("move.w %%sr,%0" : "=d"(statusRegister));
  statusRegister = (unsigned short)((statusRegister &
                                     (unsigned short)~kStatusRegisterIplMask) |
                                    (unsigned short)((maskLevel & kIplLevelMask)
                                                     << kIplShiftBits));
  __asm__ volatile("move.w %0,%%sr" ::"d"(statusRegister) : "cc");
}

static void buildTitleLine(char out[kTitleBufferSize]) {
  static const char kTitleText[] =
      "SidecarTridge Atari ST Rescue Switcher - v" APP_VERSION_STR;
  unsigned short len = 0;
  unsigned short left = 0;

  while (kTitleText[len] && len < kTitleWidthChars) {
    len++;
  }

  if (len < kTitleWidthChars) {
    left = (unsigned short)((kTitleWidthChars - len) / 2U);
  }

  for (unsigned short i = 0; i < kTitleWidthChars; ++i) {
    out[i] = ' ';
  }

  for (unsigned short i = 0; i < len; ++i) {
    out[left + i] = kTitleText[i];
  }

  out[kTitleWidthChars] = '\0';
}

static void copy_name(char dst[ROM_NAME_SIZE], const unsigned char *src) {
  unsigned int i = 0U;
  while (i + 1U < ROM_NAME_SIZE && src[i] != 0U) {
    dst[i] = (char)src[i];
    ++i;
  }
  dst[i] = '\0';
}

static __attribute__((unused)) int parse_rom_description(
    unsigned char *rom_desc_raw, unsigned char *rom_params_raw,
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
  // press_key("Press any key to exit...\r\n");
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

  *rom_desc = (rom_catalog_t *)pa_alloc(
      ((unsigned long)count) << 8); /* sizeof(rom_catalog_t) is 256 */
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
      metadata = 0x1;
    }
    if (i == (int)rescue_rom_index) {
      metadata |= 0x2;
    }
    (*rom_desc)[i].metadata = metadata;
  }

  return count;
}

static char *create_file_array(rom_catalog_t *rom_desc, int num_entries) {
  // Calculate total length needed
  int total_length = 0;
  for (int i = 0; i < num_entries; i++) {
    total_length +=
        SCREEN_H_WIDTH +
        1;  // +1 for null terminator for each SCREEN_H_WIDTH chars line
  }

  // Allocate memory for the concatenated string
  char *file_string = (char *)pa_alloc(
      (unsigned long)(total_length + 1));  // +1 for final null terminator
  if (!file_string) {
    return (char *)0;  // Memory allocation failed
  }

  // Use " XXX KB AR " for the memory info, where XXX is the size in KB, A
  // ACTIVE OR NOT, R RESCUE.
  const int custom_memory_info_size = 11;

  // Width reserved for the ROM name so that: name + memory-info ==
  // SCREEN_H_WIDTH memory-info already includes the leading space, so no extra
  // -1 here
  const int custom_rom_name_size = SCREEN_H_WIDTH - custom_memory_info_size;

  // Concatenate all names into file_string
  char *current_ptr = file_string;
  for (int i = 0; i < num_entries; i++) {
    // Safely compute the visible length of the name, even if it's not
    // null-terminated
    int trimmed_str_len = 0;
    while (trimmed_str_len < custom_rom_name_size &&
           trimmed_str_len < ROM_NAME_SIZE &&
           rom_desc[i].name[trimmed_str_len] != '\0') {
      trimmed_str_len++;
    }

    // Copy and pad the ROM name to the fixed width
    if (trimmed_str_len > 0) {
      for (int j = 0; j < trimmed_str_len; ++j) {
        current_ptr[j] = rom_desc[i].name[j];
      }
    }
    for (int j = trimmed_str_len; j < custom_rom_name_size; j++) {
      current_ptr[j] = ' ';
    }
    current_ptr += custom_rom_name_size;

    // Build fixed-width memory info: " XXX KB AR " (11 chars total)
    // Compute KB and clamp to 3 digits to avoid clipping
    unsigned long kb_val =
        ((unsigned long)BIG_ENDIAN_TO_LITTLE_ENDIAN_WORD(rom_desc[i].blocks))
        << 2;
    if (kb_val > 999UL) {
      kb_val = 999UL;  // clamp to 3 digits
    }

    // Compose exactly 11 characters
    // Leading space
    current_ptr[0] = ' ';
    // Right-justify number into 3 columns with spaces without div/mod helpers.
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
    char d1 = (d1v > 0UL) ? (char)('0' + (char)d1v) : ' ';
    char d2 = ((d1v > 0UL) || (d2v > 0UL)) ? (char)('0' + (char)d2v) : ' ';
    char d3 = (char)('0' + (char)d3v);
    current_ptr[1] = d1;
    current_ptr[2] = d2;
    current_ptr[3] = d3;
    // " KB "
    current_ptr[4] = ' ';
    current_ptr[5] = 'K';
    current_ptr[6] = 'B';
    current_ptr[7] = ' ';
    // Flags
    current_ptr[8] = ((rom_desc[i].metadata & 0x1) != 0) ? 'A' : ' ';
    current_ptr[9] = ((rom_desc[i].metadata & 0x2) != 0) ? 'R' : ' ';
    current_ptr[10] = ' ';

    current_ptr += custom_memory_info_size;

    // Null-terminate each entry string
    *current_ptr = '\0';
    current_ptr++;
  }

  file_string[total_length] = '\0';  // Add the final null terminator
  return file_string;
}

static __attribute__((unused)) char *print_file_at_index(char *current_ptr,
                                                         unsigned short index,
                                                         int num_columns) {
  (void)num_columns;
  unsigned short current_index = 0;
  while (*current_ptr) {  // As long as we don't hit the double null terminator
    if (current_index == index) {
      // Print the entire string using text_printf
      //            text_printf("\033K%s", current_ptr);
      deleteLineFromCursor();  // Erase to end of line
      text_printf(current_ptr);
      while (*current_ptr) {
        current_ptr++;
      }

      // text_printf("\r\n");
      return current_ptr;
    }

    // Skip past the current filename to the next
    while (*current_ptr) current_ptr++;
    current_ptr++;  // Skip the null terminator for the current filename
    current_index++;
  }

  text_printf("No file found at index %u\r\n", index);
  return current_ptr;
}

static __attribute__((unused)) void highlight_and_print(
    char *file_array, unsigned short index, unsigned short offset,
    int current_line, int num_columns, unsigned short highlight) {
  text_set_cursor(0, current_line + 2 + index - offset);
  if (highlight) invertVideoEnable();
  deleteLineFromCursor();  // Erase to end of line
  print_file_at_index(file_array, index, num_columns);
  if (highlight) invertVideoDisable();
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
  int page_number = 0;  // Page number starts at 0
  int current_index = 0;
  text_set_cursor(0, page_size + PAGINATED_CONTENT_Y_OFFSET);
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
    deleteLineFromCursor();  // Erase to end of line
    text_printf("%s found: %d. ", item_name, num_files);
    text_printf("Page %d of %d\r\n", page_number + 1, max_page + 1);
    text_set_cursor(0, current_line + 1);
    deleteLineFromCursor();  // Erase to end of line
    while (index <= num_files) {
      if ((start_index <= index) && (index <= end_index)) {
        // Print the index number
        text_set_cursor(0, current_line + 2 + index - start_index);
        current_ptr = print_file_at_index(file_array, index, SCREEN_H_WIDTH);
        index++;
      } else {
        // Skip past the current filename to the next
        while (*current_ptr != 0x00) current_ptr++;
        current_ptr++;  // skip the null terminator for the current filename
        index++;
      }
    }
    for (int i = index; i <= page_end_index; i++) {
      // THe -1 at the begining it's because of the previous increment
      text_set_cursor(0, -1 + current_line + 2 + i - start_index);
      deleteLineFromCursor();  // Erase to end of line
    }

    unsigned char key;
    unsigned short change_page = 0U;
    while ((selected_rom < 0) && (!change_page)) {
      highlight_and_print(file_array, current_index,
                          (unsigned short)start_index, current_line,
                          SCREEN_H_WIDTH, 1U);
      key = kbd_poll_scancode_wait();
      if (keypress != (unsigned long *)0) {
        *keypress = key;
      }
      switch (key) {
        case KEY_UP_ARROW:
          if (current_index > start_index) {
            highlight_and_print(file_array, current_index,
                                (unsigned short)start_index, current_line,
                                SCREEN_H_WIDTH, 0U);
            current_index = current_index - 1;
          }
          break;

        case KEY_DOWN_ARROW:
          if (current_index < end_index) {
            highlight_and_print(file_array, current_index,
                                (unsigned short)start_index, current_line,
                                SCREEN_H_WIDTH, 0U);
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
          /* Avoid duplicate case labels if KEY_ENTER == KEY_RETURN (e.g.,
           * AmigaOS) */
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
          // Handle other keys
          if (keypress != (unsigned long *)0) {
            *keypress = key;
          }
          // Do not return, but store the key pressed, if any.
          break;
      }

#if defined(_DEBUG) && (_DEBUG > 0)
      text_set_cursor(0, PAGINATED_CONTENT_Y_OFFSET + page_size);
      text_printf("Key: 0x%02x    Index: %d", (unsigned int)key, current_index);
#endif
    }
  }
  return -1;  // Return -1 if no selection was made
}

void rom_switcher_main(void) {
  pa_init(ST_RAM_VARS_BASE_ADDR + (unsigned long)kHeapReservedBytes,
          ST_RAM_VARS_SIZE_BYTES - kHeapReservedBytes);

  /* Polled runtime: mask all IRQ levels (IPL=7). */
  setInterruptLevelMask(kPolledInterruptLevel);

#if defined(_DEBUG) && (_DEBUG > 0)
  hatari_trace_init();
  hatari_trace_msg("[romswdbg] boot\n");
#endif
  char titleLine[kTitleBufferSize];

  screen_init();
  const unsigned char medium = screen_is_medium_mode();

  screen_set_display_palette();
  text_init();

  text_clear();
  text_set_color((unsigned char)(medium ? kColorDefault : kColorDefault));
  buildTitleLine(titleLine);

  text_printf("Loading available ROM images...");

  text_printf("\r\n");

#define MEMORY_EXCHANGE_SIZE 61440        // 60KBytes
#define MEMORY_PARAMS_EXCHANGE_SIZE 4096  // 4096 bytes

  // Memory to store the information about the ROM files
  unsigned char *buffer =
      (unsigned char *)pa_alloc_aligned(MEMORY_EXCHANGE_SIZE, 16UL);
  if (!buffer) {
    text_printf(
        "OOM error allocating memory for the ROM buffer %lu-byte aligned "
        "buffer.\r\n",
        (unsigned long)MEMORY_EXCHANGE_SIZE);
    return;  // Error
  }

  // Memory to store the information about the parameters of the ROM files
  unsigned char *buffer_params =
      (unsigned char *)pa_alloc_aligned(MEMORY_PARAMS_EXCHANGE_SIZE, 16UL);
  if (!buffer_params) {
    text_printf(
        "OOM error allocating memory for the ROM parameters buffer %lu-byte "
        "aligned buffer.\r\n",
        (unsigned long)MEMORY_PARAMS_EXCHANGE_SIZE);
    return;  // Error
  }

  rom_catalog_t *rom_descriptions;

  // Init ROMs list
  // unsigned long free_space_kbytes = 0;
  int num_entries = 0;
  int flow_control = 0;
  int rom_number = 0;
  signed short protocol_version = 0;

#if defined(_DEBUG) && (_DEBUG > 0)
  init_rom_address(ROM_BASE_ADDR_UL, hatari_trace_msg);
#else
  init_rom_address(ROM_BASE_ADDR_UL, (helper_trace_fn_t)0);
#endif

#if (defined(_TEST) && (_TEST > 0))
  protocol_version = 1;
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
  // Read the ROM parameters from the SidecarTOS
  // Without the SidecarTOS, will read only garbage
  // The ENDIAN_BIG is used to swap the bytes in the buffer
  // When reading directly from the flash, the bytes must be swapped
  int err = read_flash_page(buffer_params, FLASH_PARAMS_START, ENDIAN_BIG);
  if (err != 0) {
    text_printf("Error reading the flash page at address %08lx. Exiting.\r\n",
                (unsigned long)FLASH_PARAMS_START);
    kbd_wait_for_key_press();

    return;
  }

  // Read the ROM configuration from the SidecarTOS
  // Without the SidecarTOS, will read only garbage

  const unsigned long pages = MEMORY_EXCHANGE_SIZE / READ_ROM_PAGE_SIZE;
  for (unsigned long i = 0U; i < pages; i++) {
    unsigned long addr =
        (unsigned long)(FLASH_CATALOG_START +
                        (unsigned long)i * (unsigned long)READ_ROM_PAGE_SIZE);
    int err =
        read_flash_page(buffer + i * READ_ROM_PAGE_SIZE, addr, ENDIAN_BIG);
    if (err != 0) {
      text_printf("Error reading the flash page at address %08lx. Exiting.\r\n",
                  (unsigned long)addr);
      kbd_wait_for_key_press();

      return;
    }
  }

  protocol_version = buffer_params[ROM_DESCRIPTION_SIZE * 2] +
                     (buffer_params[1 + (ROM_DESCRIPTION_SIZE * 2)] * 256);

  num_entries = parse_rom_description(buffer, buffer_params, &rom_descriptions);

#endif
  (void)protocol_version;

  char *file_array = create_file_array(rom_descriptions, num_entries);
  (void)file_array;

  if (protocol_version != SWITCHER_TOS_PROTOCOL_VERSION) {
    text_printf(
        "This version of SWITCHER tool is not compatible with the firmware of "
        "the device.\r\n");
    text_printf("Press any key to exit...\r\n");
    kbd_wait_for_key_press();
    return;
  }

  while (flow_control == 0) {
    text_clear();
    text_set_color((unsigned char)(medium ? kColorDefault : kColorDefault));
    buildTitleLine(titleLine);

    /* VT-52: cursor to row 0 col 0, reverse on, print 80-char centered title,
     * reverse off. */
    text_printf("\x1BY  \x1Bp%s\x1Bq", titleLine);

    /* VT-52: move to row 2 col 0 before rendering menu entries. */
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

      flow_control = 1;  // Exit menu loop and change the ROM
    }
  }

  deleteLineFromCursor();
  text_printf("Rebooting the computer with the new ROM...");
  send_change_rom_command_and_hard_reset((unsigned char)(rom_number - 1));
}
