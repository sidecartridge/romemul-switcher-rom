/**
 * File: src/common/kbd.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared keyboard input definitions.
 */

#pragma once

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

unsigned char kbd_poll_scancode(void);
unsigned char kbd_poll_scancode_wait(void);
void kbd_wait_for_key_press(void);
unsigned char kbd_wait_for_key_or_esc(void);
