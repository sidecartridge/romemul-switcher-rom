#include "mem.h"

#ifndef STARTUP_ROM_OBJ
#define STARTUP_ROM_OBJ startup_ste.s.o
#endif

ENTRY(_os_entry)

MEMORY
{
  ROM (rx)  : ORIGIN = ROM_BASE_ADDR_UL, LENGTH = 0x01000000
  RAM (rwx) : ORIGIN = ST_RAM_VARS_ALLOC_BASE_ADDR_UL, LENGTH = ST_RAM_VARS_ALLOC_SIZE_BYTES
}

SECTIONS
{
  .boot ORIGIN(ROM) : AT(0)
  {
    __rom_vma_start = .;
    *STARTUP_ROM_OBJ(.text .text.*)
  } > ROM

  . = ALIGN(4);
  __rom_boot_end = .;

  .ramtext ST_RAM_EXEC_BASE_ADDR_UL : AT(LOADADDR(.boot) + SIZEOF(.boot))
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
  _edata = .;
  __edata = .;

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

  __rom_lma_end = LOADADDR(.data) + SIZEOF(.data);

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
  }
}
