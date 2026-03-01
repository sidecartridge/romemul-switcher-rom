#include "rom_switcher.h"
#include "../common/romemul_hw.h"

static const uint16_t kBankPalette[] = {
    COLOR_BANK0,
    COLOR_BANK1,
    COLOR_BANK2,
    COLOR_BANK3
};

static void delay(uint32_t loops)
{
    while (loops--) {
        __asm__ volatile ("nop\n nop\n" ::: "memory");
    }
}

static void apply_bank(uint8_t bank)
{
    romemul_select_bank(bank);
    romemul_wait_ready();
    st_set_border(kBankPalette[bank & 0x03]);
}

static uint8_t map_scancode(uint8_t code)
{
    switch (code) {
        case 0x02: return 0; // '1'
        case 0x03: return 1; // '2'
        case 0x04: return 2; // '3'
        case 0x05: return 3; // '4'
        default:   return 0xFF;
    }
}

void rom_switcher_main(void)
{
    uint8_t current_bank = 0;

    st_draw_banner();
    apply_bank(current_bank);

    for (;;) {
        uint8_t scan = st_poll_scancode();
        if (scan == 0xFF) {
            delay(2500);
            continue;
        }

        if (scan & 0x80) {
            // ignore key releases
            continue;
        }

        uint8_t mapped = map_scancode(scan);
        if (mapped != 0xFF && mapped != current_bank) {
            current_bank = mapped;
            apply_bank(current_bank);
        }
    }
}

uint8_t st_poll_scancode(void)
{
    if (!(ST_IKBD_STATUS & IKBD_STATUS_RX_FULL)) {
        return 0xFF;
    }
    return ST_IKBD_DATA;
}

void st_set_border(uint16_t color)
{
    ST_BORDER_COLOR = color & 0x0777;
}

void st_wait_vbl(void)
{
    volatile uint16_t *const mfp_vbl = (volatile uint16_t *)0xFFFA06;
    while (!(*mfp_vbl & 0x0080)) {
        // wait
    }
}

void st_draw_banner(void)
{
    // crude visual feedback: flash palette entries proportional to bank number
    for (uint8_t i = 0; i < 4; ++i) {
        ST_PALETTE_BASE[i] = kBankPalette[i];
    }
}
