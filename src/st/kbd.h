#pragma once

#include <stdint.h>

uint8_t kbd_poll_scancode(void);
void kbd_wait_for_key_press(void);
