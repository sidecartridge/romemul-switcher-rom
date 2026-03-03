#include "commands.h"

enum {
  kRomInRamSwapOffset = 0x4,
  kMaxCommandRetries = 16,
  kEndianBig = 1,
  kReadRomPageSize = 4096,
  kReadRomBlockSize = 512,
  kReadBlocksPerPage = (kReadRomPageSize / kReadRomBlockSize),
  kChecksumOffset = kReadRomBlockSize,
  kCmdSelectRom = 0x0000,
  kCmdUnlockReadBlockMemory = 0x0008,
  kCmdLockReadBlockMemory = 0x000A
};

static inline unsigned short be_to_le_word(unsigned short x) {
  return (unsigned short)(((x & 0xFF00U) >> 8) | ((x & 0x00FFU) << 8));
}

static unsigned long remote_rom_address = 0;
static unsigned long scratch_rom_address = 0;
static unsigned long magic_number_address = 0;
static unsigned long previous_magic_number_value = 0;
static helper_trace_fn_t g_trace_fn = (helper_trace_fn_t)0;

static unsigned long seed = 1;

static unsigned short div_u32_u16(unsigned long n, unsigned short d) {
  unsigned short q = 0;
  if (d == 0U) {
    return 0U;
  }
  while (n >= (unsigned long)d) {
    n -= (unsigned long)d;
    ++q;
  }
  return q;
}

// Simple LCG: returns a pseudo-random number
static inline unsigned long random_generator() {
  // Xorshift32 variant: avoids libgcc 32-bit multiply helpers.
  seed ^= (seed << 13);
  seed ^= (seed >> 17);
  seed ^= (seed << 5);
  return seed & 0x7fffffffU;
}
// Seed the random number generator (you can set this to any value you want)
static inline void srand_generator(unsigned long s) { seed = s; }

static inline unsigned long get_system_time_seed(void) {
  unsigned long s = 0xA5A5A5A5UL;

  for (unsigned long i = 0; i < 16UL; i++) {
    unsigned long tc = (unsigned long)(*(
        volatile unsigned char *)0xFFFA23UL); /* Timer C data */

    unsigned long tcd = (unsigned long)(*(
        volatile unsigned char *)0xFFFA1DUL); /* Timer C&D control */

    s ^= (tc << ((i & 3UL) * 8UL));

    /* rotate left 5 bits */
    s = (s << 5) | (s >> 27);

    s ^= (tcd * 0x9EUL);
  }

  /* Optional: mix Timer D data if running */
  s ^= ((unsigned long)(*(volatile unsigned char *)0xFFFA25UL) << 16);

  return s;
}

static void copy_bytes(unsigned char *dst, const unsigned char *src,
                       unsigned short len) {
  unsigned short i;
  for (i = 0; i < len; ++i) {
    dst[i] = src[i];
  }
}

static void trace_msg(const char *message) {
  if (g_trace_fn != (helper_trace_fn_t)0 && message != (const char *)0) {
    g_trace_fn(message);
  }
}

static char hex_nibble(unsigned char value) {
  if (value < 10U) {
    return (char)('0' + (char)value);
  }
  return (char)('A' + (char)(value - 10U));
}

static void trace_u32_hex(const char *name, unsigned long value) {
  char line[96];
  int pos = 0;
  int i = 0;
  static const char kPrefix[] = "[romswdbg] helper: ";

  while (kPrefix[i] != '\0' && pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = kPrefix[i++];
  }

  i = 0;
  while (name[i] != '\0' && pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = name[i++];
  }

  if (pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = '=';
  }
  if (pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = '0';
  }
  if (pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = 'x';
  }

  for (i = 28; i >= 0 && pos < (int)(sizeof(line) - 1U); i -= 4) {
    line[pos++] = hex_nibble((unsigned char)((value >> i) & 0x0FU));
  }

  if (pos < (int)(sizeof(line) - 1U)) {
    line[pos++] = '\n';
  }
  line[pos] = '\0';
  trace_msg(line);
}

void init_rom_address(unsigned long rom_base_address,
                      helper_trace_fn_t trace_fn) {
  g_trace_fn = trace_fn;
  unsigned long addr = (unsigned long)rom_base_address;
  magic_number_address = addr;
  scratch_rom_address = addr;
  remote_rom_address = addr + kRomInRamSwapOffset;
  previous_magic_number_value = *((volatile unsigned long *)(addr));
  trace_u32_hex("rom_base_address", addr);
  trace_u32_hex("magic_number_address", magic_number_address);
  trace_u32_hex("scratch_rom_address", scratch_rom_address);
  trace_u32_hex("remote_rom_address", remote_rom_address);
  trace_u32_hex("previous_magic_number_value", previous_magic_number_value);
  trace_msg("[romswdbg] helper: init_rom_address done\n");
}

// Function to encode an 8-bit byte to a 12-bit Hamming code
static inline unsigned short hamming_encode(unsigned char data) {
  unsigned short encoded = 0;

  // Place the data bits in the correct positions
  // Positions: 3, 5, 6, 7, 9, 10, 11, 12
  encoded |= ((data >> 0) & 0x01) << 2;   // Data bit 0 to position 3
  encoded |= ((data >> 1) & 0x01) << 4;   // Data bit 1 to position 5
  encoded |= ((data >> 2) & 0x01) << 5;   // Data bit 2 to position 6
  encoded |= ((data >> 3) & 0x01) << 6;   // Data bit 3 to position 7
  encoded |= ((data >> 4) & 0x01) << 8;   // Data bit 4 to position 9
  encoded |= ((data >> 5) & 0x01) << 9;   // Data bit 5 to position 10
  encoded |= ((data >> 6) & 0x01) << 10;  // Data bit 6 to position 11
  encoded |= ((data >> 7) & 0x01) << 11;  // Data bit 7 to position 12

  // Calculate parity bits
  unsigned char p1 = ((encoded >> 2) & 1) ^ ((encoded >> 4) & 1) ^
                     ((encoded >> 6) & 1) ^ ((encoded >> 8) & 1) ^
                     ((encoded >> 10) & 1);
  unsigned char p2 = ((encoded >> 2) & 1) ^ ((encoded >> 5) & 1) ^
                     ((encoded >> 6) & 1) ^ ((encoded >> 9) & 1) ^
                     ((encoded >> 10) & 1);
  unsigned char p4 = ((encoded >> 4) & 1) ^ ((encoded >> 5) & 1) ^
                     ((encoded >> 6) & 1) ^ ((encoded >> 11) & 1);
  unsigned char p8 = ((encoded >> 8) & 1) ^ ((encoded >> 9) & 1) ^
                     ((encoded >> 10) & 1) ^ ((encoded >> 11) & 1);

  // Set the parity bits in the encoded word
  encoded |= (p1 << 0);  // Parity bit at position 1
  encoded |= (p2 << 1);  // Parity bit at position 2
  encoded |= (p4 << 3);  // Parity bit at position 4
  encoded |= (p8 << 7);  // Parity bit at position 8

  return encoded;
}

static inline unsigned long send_magic_sequence_asm(unsigned long rom_addr,
                                                    unsigned short command,
                                                    unsigned short param) {
  (void)rom_addr;

  // Send random magic number
  srand_generator(get_system_time_seed());
  unsigned long magic_number = random_generator() & 0xFFFEFFFE;
  unsigned short magic_number_lsb = magic_number & 0xFFFF;
  unsigned short magic_number_msb = ((magic_number >> 16) & 0xFFFF);

  unsigned short checksum = command;
  checksum += param;
  checksum += magic_number_lsb;
  checksum += magic_number_msb;
  checksum = checksum & 0xFFFE;

  if (magic_number_address == 0xFC0000) {
    // Send the MAGIC SEQUENCE to the ROM for ST ROMS
    asm volatile(
        "move.l #0xFC0000, %%d1\n\t"
        "move.l %%d1, %%d2\n\t"
        "move.l %%d1, %%d3\n\t"
        "move.l %%d1, %%d4\n\t"
        "move.l %%d1, %%d5\n\t"
        "\n\t"
        "move.w %0, %%d1\n\t"
        "move.l %%d1, %%a0\n\t"
        "\n\t"
        "move.w %1, %%d2\n\t"
        "move.l %%d2, %%a1\n\t"
        "\n\t"
        "move.w %2, %%d3\n\t"
        "move.l %%d3, %%a2\n\t"
        "\n\t"
        "move.w %3, %%d4\n\t"
        "move.l %%d4, %%a3\n\t"
        "\n\t"
        "move.w %4, %%d5\n\t"
        "move.l %%d5, %%a4\n\t"
        "\n\t"
        "move.b 0xFC1234, %%d0\n\t"
        "move.b 0xFCFC42, %%d0\n\t"
        "move.b 0xFC6452, %%d0\n\t"
        "move.b 0xFCCDE0, %%d0\n\t"
        "move.b 0xFC5CA2, %%d0\n\t"
        "move.b 0xFC8CA4, %%d0\n\t"
        "move.b 0xFC1F94, %%d0\n\t"
        "move.b 0xFCE642, %%d0\n\t"
        "move.b (%%a0),%%d1\n\t"
        "move.b (%%a1),%%d2\n\t"
        "move.b (%%a2),%%d3\n\t"
        "move.b (%%a3),%%d4\n\t"
        "move.b (%%a4),%%d5\n\t"

        "\n\t"
        :
        : "m"(magic_number_msb), "m"(magic_number_lsb), "m"(command),
          "m"(param), "m"(checksum)
        : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "a2", "a3", "a4");
  }
  if (magic_number_address == 0xE00000) {
    // Send the MAGIC SEQUENCE to the ROM for STE and MegaSTE ROMS
    asm volatile(
        "move.l #0xE00000, %%d1\n\t"
        "move.l %%d1, %%d2\n\t"
        "move.l %%d1, %%d3\n\t"
        "move.l %%d1, %%d4\n\t"
        "move.l %%d1, %%d5\n\t"
        "\n\t"
        "move.w %0, %%d1\n\t"
        "move.l %%d1, %%a0\n\t"
        "\n\t"
        "move.w %1, %%d2\n\t"
        "move.l %%d2, %%a1\n\t"
        "\n\t"
        "move.w %2, %%d3\n\t"
        "move.l %%d3, %%a2\n\t"
        "\n\t"
        "move.w %3, %%d4\n\t"
        "move.l %%d4, %%a3\n\t"
        "\n\t"
        "move.w %4, %%d5\n\t"
        "move.l %%d5, %%a4\n\t"
        "\n\t"
        "move.b 0xE01234, %%d0\n\t"
        "move.b 0xE0FC42, %%d0\n\t"
        "move.b 0xE06452, %%d0\n\t"
        "move.b 0xE0CDE0, %%d0\n\t"
        "move.b 0xE05CA2, %%d0\n\t"
        "move.b 0xE08CA4, %%d0\n\t"
        "move.b 0xE01F94, %%d0\n\t"
        "move.b 0xE0E642, %%d0\n\t"
        "move.b (%%a0),%%d1\n\t"
        "move.b (%%a1),%%d2\n\t"
        "move.b (%%a2),%%d3\n\t"
        "move.b (%%a3),%%d4\n\t"
        "move.b (%%a4),%%d5\n\t"

        "\n\t"
        :
        : "m"(magic_number_msb), "m"(magic_number_lsb), "m"(command),
          "m"(param), "m"(checksum)
        : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "a2", "a3", "a4");
  }
  return magic_number;
}

static unsigned long send_async_rom_command(unsigned short param,
                                            unsigned short command) {
  unsigned long magic_number =
      send_magic_sequence_asm(scratch_rom_address, command, param << 1);

  return magic_number;
}

static int send_sync_rom_command(unsigned short param, unsigned short command,
                                 unsigned short timeout) {
  // Save the previous value where the magic number was stored
  volatile unsigned long *magic_number_ptr =
      (unsigned long *)(magic_number_address);

  // Send the MAGIC SEQUENCE to the ROM
  unsigned long magic_number =
      send_magic_sequence_asm(scratch_rom_address, command, param << 1);

  // Now we wait until the magic number is read consistently
  unsigned long current_magic_number = *magic_number_ptr;

  // Set the timeout
  unsigned short num_tries = timeout;

  while (num_tries-- > 0) {
    // If the magic number is 0xFFFFFFD, then there was a checksum error
    if (current_magic_number == 0xFFFFFFFD) {
      return -2;  // Checksum error
    }

    // Early exit on match
    if (magic_number == current_magic_number) {
      return 0;  // Success
    }

    // Read the magic number once per iteration
    current_magic_number = *magic_number_ptr;
  }

  return -1;  // Timeout
}

void send_change_rom_command_and_hard_reset(unsigned char rom_index) {
  send_magic_sequence_asm(scratch_rom_address, kCmdSelectRom,
                          hamming_encode(rom_index) << 1);

  // Disable all interrupts, wait some seconds and finally do a hard reset
  __asm__(
      "move.w #0x2700, %sr\n\t"
      "move.l #0xFFFFF, %d0\n\t"
      "1: nop\n\t"
      "subq.l #1, %d0\n\t"
      "bne.s 1b\n\t"
      "clr.l 0x00000420\n\t"  // Invalidate memory system variables
      "clr.l 0x0000043A\n\t"
      "clr.l 0x0000051A\n\t"
      "move.l (0x4), %a0\n\t"  // Now we can safely jump to the reset vector
      "jmp (%a0)");
}

int read_flash_block(void *memory_exchange, unsigned long flash_address_offset,
                     unsigned short block_size) {
  unsigned short flash_block_number =
      div_u32_u16(flash_address_offset, block_size);

  int error = 0;

  unsigned short remote_checksum = 0;
  unsigned short num_tries = kMaxCommandRetries;

  // Store volatile pointer to magic number for easier access
  volatile unsigned long *magic_number_ptr =
      (volatile unsigned long *)(magic_number_address);
  unsigned long previous_magic_number = *magic_number_ptr;

  // Store volatile pointer to checksum location
  volatile unsigned short *remote_checksum_ptr =
      (volatile unsigned short *)(remote_rom_address + kChecksumOffset);

  while (num_tries-- > 0) {
    error = send_sync_rom_command(flash_block_number, kCmdUnlockReadBlockMemory,
                                  0x4000);
    if (error != 0) {
      trace_msg(
          "[romswdbg] helper: unlock read block failed in read_flash_block\n");
    } else if (num_tries > 0) {
      // printf("Copying the flash page %d at address %x.\r\n",
      // flash_block_number, flash_address_offset);

      // Coyiing the data from the ROM to the memory exchange
      copy_bytes((unsigned char *)memory_exchange,
                 (const unsigned char *)remote_rom_address, block_size);

      // We can now copy the data from the unlocked hole in the memory exchange
      unsigned short checksum = 0;
      unsigned short i;
      for (i = 0; i < block_size; i += 2) {
        unsigned short value =
            *((unsigned short *)((unsigned char *)memory_exchange + i));
        checksum += value;
      }

      // Read the checksum only once per iteration
      remote_checksum = *remote_checksum_ptr;
      if (checksum != remote_checksum) {
        trace_msg("[romswdbg] helper: checksum mismatch in read_flash_block\n");
      } else {
        // Exit the loop if the checksum matches
        break;
      }
    }
  }
  if (num_tries == 0) {
    trace_msg("[romswdbg] helper: read_flash_block failed\n");
    error = -1;
  }

  // Restore the magic-number location after unlocking a block.
  num_tries = kMaxCommandRetries;
  while (num_tries-- > 0) {
    // Function calls might have overhead, so depending on the system, you may
    // also want to inline
    send_async_rom_command(flash_block_number, kCmdLockReadBlockMemory);

    unsigned long current_magic_number = 0;
    unsigned short num_tries_inner = 0xFFFF;
    while (num_tries_inner-- > 0) {
      // Re-read the magic number only once per loop iteration
      current_magic_number = *magic_number_ptr;
      if (current_magic_number == previous_magic_number) {
        break;
      }
      if (current_magic_number == 0xFFFFFFFD) {
        break;
      }
    }
    if (current_magic_number == previous_magic_number) {
      break;
    }
  }
  if (num_tries == 0) {
    trace_msg("[romswdbg] helper: restore magic-number failed\n");
    error = -1;
  }
  return error;
}

int read_flash_page(void *memory_exchange, unsigned long flash_address_offset,
                    unsigned char endian) {
  // A page is kReadRomPageSize composed of kReadRomBlockSize.
  trace_msg("[romswdbg] helper: read_flash_page begin\n");
  for (unsigned short i = 0; i < kReadBlocksPerPage; i++) {
    if (read_flash_block(
            (unsigned char *)memory_exchange + i * kReadRomBlockSize,
            flash_address_offset + i * kReadRomBlockSize,
            kReadRomBlockSize) != 0) {
      trace_msg("[romswdbg] helper: read_flash_page failed on one block\n");
      return -1;
    }
    if (endian == (unsigned char)kEndianBig) {
      trace_msg("[romswdbg] helper: read_flash_page endian swap\n");
      for (unsigned short j = 0; j < kReadRomBlockSize; j = j + 2) {
        *((unsigned short *)((unsigned char *)memory_exchange +
                             i * kReadRomBlockSize + j)) =
            be_to_le_word(
                *((unsigned short *)((unsigned char *)memory_exchange +
                                     i * kReadRomBlockSize + j)));
      }
    }
  }
  trace_msg("[romswdbg] helper: read_flash_page done\n");
  return 0;
}
