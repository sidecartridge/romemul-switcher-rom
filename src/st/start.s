.globl _start
.extern rom_switcher_main

.text
_start:
    lea     stack_top, %sp
    jsr     rom_switcher_main

halt:
    bra.s   halt

.bss
    .align  2
stack_area:
    .space  4096
stack_top:
