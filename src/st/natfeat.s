    .text

    .globl _nf_get_id
_nf_get_id:
    .short 0x7300
    rts

    .globl _nf_call
_nf_call:
    .short 0x7301
    rts
