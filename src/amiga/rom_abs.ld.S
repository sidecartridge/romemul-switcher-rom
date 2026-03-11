#include "mem.h"

ENTRY(_amiga_boot)

#ifndef STARTUP_ROM_OBJ
#define STARTUP_ROM_OBJ startup.s.o
#endif

MEMORY
{
  ROM (rx)  : ORIGIN = ROM_BASE_ADDR_UL, LENGTH = AMIGA_ROM_SIZE_BYTES
  RAM (rwx) : ORIGIN = AMIGA_RAM_VARS_ALLOC_BASE_ADDR_UL, LENGTH = AMIGA_RAM_VARS_ALLOC_SIZE_BYTES
}

SECTIONS
{
  .rom_header ORIGIN(ROM) : AT(0)
  {
    __rom_vma_start = .;
    SHORT(0x1114)
    SHORT(0x4EF9)
    LONG(_amiga_boot)
    LONG(0x0000FFFF)
    SHORT(0x0033)
    SHORT(0x0001)
    SHORT(0x0033)
    SHORT(0x0001)
    LONG(0xFFFFFFFF)
  } > ROM

  . = ROM_BASE_ADDR_UL + 0xD0;
  .boot ROM_BASE_ADDR_UL + 0xD0 : AT(0xD0)
  {
    *STARTUP_ROM_OBJ(.text .text.*)
  } > ROM

  . = ALIGN(4);
  __rom_boot_end = .;

  .ramtext AMIGA_RAM_EXEC_BASE_ADDR_UL : AT(LOADADDR(.boot) + SIZEOF(.boot))
  {
    __ram_code_load_start = LOADADDR(.ramtext);
    __ram_code_rom_start = ABSOLUTE(ORIGIN(ROM) + LOADADDR(.ramtext));
    __ram_code_start = .;
    *(EXCLUDE_FILE(*STARTUP_ROM_OBJ) .text .text.*)
    *(.rodata .rodata.*)
    __ram_code_end = .;
  } > RAM

  . = ALIGN(4);
  _etext = .;
  __etext = .;

  .data : AT(LOADADDR(.ramtext) + SIZEOF(.ramtext))
  {
    __data_load_start = LOADADDR(.data);
    __data_rom_start = ABSOLUTE(ORIGIN(ROM) + LOADADDR(.data));
    __data_start = .;
    *(.data .data.*)
    __data_end = .;
  } > RAM

  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > RAM

  _end = .;
  __end = .;

  __rom_payload_end = LOADADDR(.data) + SIZEOF(.data);
  ASSERT(__rom_payload_end <= AMIGA_ROM_KICKETY_OFFSET_BYTES,
         "Amiga ROM payload overlaps kickety split area")
  ASSERT(_end <= AMIGA_SCREEN_BASE_ADDR_UL,
         "Amiga RAM sections overlap the frame buffer")

  .kickety_split (ROM_BASE_ADDR_UL + AMIGA_ROM_KICKETY_OFFSET_BYTES) :
      AT(AMIGA_ROM_KICKETY_OFFSET_BYTES)
  {
    SHORT(0x1111)
    SHORT(0x4EF9)
    LONG(ROM_BASE_ADDR_UL + 2)
  } > ROM

  .rom_footer (ROM_BASE_ADDR_UL + AMIGA_ROM_FOOTER_OFFSET_BYTES) :
      AT(AMIGA_ROM_FOOTER_OFFSET_BYTES)
  {
    LONG(0x00000000)
    LONG(AMIGA_ROM_SIZE_BYTES)
    SHORT(0x0018)
    SHORT(0x0019)
    SHORT(0x001A)
    SHORT(0x001B)
    SHORT(0x001C)
    SHORT(0x001D)
    SHORT(0x001E)
    SHORT(0x001F)
  } > ROM

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
  }
}
