#include "rom_switcher.h"
#include "../common/romemul_hw.h"
#include "text.h"

static const uint16_t kForegroundColors[] = {
    COLOR_LIGHT,
    COLOR_DARK
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
    st_set_palette_pair(kForegroundColors[bank & 0x01]);
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

    text_init();
    st_draw_banner();
    apply_bank(current_bank);
    text_print_menu(current_bank);

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
            text_print_menu(current_bank);
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

void st_set_palette_pair(uint16_t fg_color)
{
    ST_PALETTE_BASE[ST_PALETTE_BG_INDEX] = COLOR_DARK;
    ST_PALETTE_BASE[ST_PALETTE_FG_INDEX] = fg_color & 0x0777;
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
    st_set_palette_pair(COLOR_LIGHT);
}
