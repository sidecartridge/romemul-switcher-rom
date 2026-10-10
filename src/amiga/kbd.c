/**
 * File: src/amiga/kbd.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga keyboard input handling.
 */

#include "../common/kbd.h"
#include "hw.h"

enum {
  kCiaSpInterruptBit = 0x08U,
  kCiaSerialOutputModeBit = 0x40U,
  kInvalidScancode = 0xFFU,
  kKeyReleaseMask = 0x80U,
  kPollDelayLoops = 4096U,
  kHandshakeDelayLoops = 192U,
  kRawEnter = 0x43U,
  kRawReturn = 0x44U,
  kRawEsc = 0x45U,
  kRawUp = 0x4CU,
  kRawDown = 0x4DU,
  kRawRight = 0x4EU,
  kRawLeft = 0x4FU,
  kRawR = 0x13U,
  kRawT = 0x14U,
  kRawS = 0x21U,
  kRawU = 0x16U,
  kRawD = 0x22U,
  kRawM = 0x37U,
  kRawResetWarning = 0x78U
};

static unsigned char kbdInitialized = 0U;

#if defined(_DEBUG) && (_DEBUG > 0)
/*
 * Debug builds also take keys from Paula's serial port, which is how the
 * FS-UAE harness types: FS-UAE has no other way to inject keys (EPIC-00
 * STORY-02). One byte per key: 0x1B ESC, 0x0D RETURN, 0x80 to 0x83 the up,
 * down, left and right arrows, 'd' 'r' 'm' 'u' 't' 's' those letters; any other byte
 * counts as a plain key press.
 */
enum {
  kSerdatrRxBufferFull = 0x4000U,
  kIntreqRxBufferFull = 0x0800U,
  kSerialDataMask = 0x00FFU,
  kSerialEsc = 0x1BU,
  kSerialReturn = 0x0DU,
  kSerialUp = 0x80U,
  kSerialDown = 0x81U,
  kSerialLeft = 0x82U,
  kSerialRight = 0x83U
};

static unsigned char kbdSerialPoll(unsigned char *byte_out) {
  const unsigned short serdatr = AMIGA_SERDATR;

  if ((serdatr & kSerdatrRxBufferFull) == 0U) {
    return 0U;
  }
  *byte_out = (unsigned char)(serdatr & kSerialDataMask);
  AMIGA_INTREQ = kIntreqRxBufferFull;
  return 1U;
}

static unsigned char map_serial_to_logical(unsigned char byte) {
  switch (byte) {
    case kSerialEsc:
      return KEY_ESC;
    case kSerialReturn:
      return KEY_RETURN;
    case kSerialUp:
      return KEY_UP_ARROW;
    case kSerialDown:
      return KEY_DOWN_ARROW;
    case kSerialLeft:
      return KEY_LEFT_ARROW;
    case kSerialRight:
      return KEY_RIGHT_ARROW;
    case 'd':
      return KEY_D;
    case 'r':
      return KEY_R;
    case 'm':
      return KEY_M;
    case 'u':
      return KEY_U;
    case 't':
      return KEY_T;
    case 's':
      return KEY_S;
    default:
      return kInvalidScancode;
  }
}
#endif

static void kbdDelay(unsigned long loops) {
  while (loops--) {
    __asm__ volatile("nop\n nop\n nop\n nop\n" ::: "memory");
  }
}

static void kbdHandshakePulse(void) {
  AMIGA_CIAA_CRA = (unsigned char)(AMIGA_CIAA_CRA | kCiaSerialOutputModeBit);
  kbdDelay(kHandshakeDelayLoops);
  AMIGA_CIAA_CRA =
      (unsigned char)(AMIGA_CIAA_CRA & (unsigned char)~kCiaSerialOutputModeBit);
}

static void kbdInit(void) {
  if (kbdInitialized) {
    return;
  }

  AMIGA_CIAA_SDR = 0x00U;
  (void)AMIGA_CIAA_ICR;
  kbdInitialized = 1U;
}

static unsigned char kbdPollRawEvent(unsigned char *raw_code_out,
                                     unsigned char *key_down_out) {
  unsigned char encoded;

  kbdInit();

  if ((AMIGA_CIAA_ICR & kCiaSpInterruptBit) == 0U) {
    return 0U;
  }

  encoded = (unsigned char)~AMIGA_CIAA_SDR;
  kbdHandshakePulse();

  *key_down_out = (unsigned char)((encoded & 0x01U) == 0U);
  *raw_code_out = (unsigned char)(encoded >> 1);
  if (*raw_code_out >= kRawResetWarning) {
    return 0U;
  }

  return 1U;
}

static unsigned char map_raw_to_logical(unsigned char raw_code) {
  switch (raw_code) {
    case kRawUp:
      return KEY_UP_ARROW;
    case kRawDown:
      return KEY_DOWN_ARROW;
    case kRawLeft:
      return KEY_LEFT_ARROW;
    case kRawRight:
      return KEY_RIGHT_ARROW;
    case kRawReturn:
      return KEY_RETURN;
    case kRawEnter:
      return KEY_ENTER;
    case kRawEsc:
      return KEY_ESC;
    case kRawD:
      return KEY_D;
    case kRawM:
      return KEY_M;
    case kRawU:
      return KEY_U;
    case kRawR:
      return KEY_R;
    case kRawT:
      return KEY_T;
    case kRawS:
      return KEY_S;
    default:
      return kInvalidScancode;
  }
}

unsigned char kbd_poll_scancode(void) {
  unsigned char raw_code;
  unsigned char key_down;
  unsigned char logical;

#if defined(_DEBUG) && (_DEBUG > 0)
  {
    unsigned char byte;
    if (kbdSerialPoll(&byte)) {
      return map_serial_to_logical(byte);
    }
  }
#endif

  if (!kbdPollRawEvent(&raw_code, &key_down)) {
    return kInvalidScancode;
  }

  logical = map_raw_to_logical(raw_code);
  if (logical == kInvalidScancode) {
    return kInvalidScancode;
  }

  if (!key_down) {
    return (unsigned char)(logical | kKeyReleaseMask);
  }
  return logical;
}

unsigned char kbd_poll_scancode_wait(void) {
  for (;;) {
    const unsigned char scan = kbd_poll_scancode();
    if (scan == kInvalidScancode) {
      kbdDelay(kPollDelayLoops);
      continue;
    }
    if ((scan & kKeyReleaseMask) != 0U) {
      continue;
    }
    return scan;
  }
}

void kbd_wait_for_key_press(void) {
  for (;;) {
    unsigned char raw_code;
    unsigned char key_down;

#if defined(_DEBUG) && (_DEBUG > 0)
    if (kbdSerialPoll(&raw_code)) {
      return;
    }
#endif

    if (!kbdPollRawEvent(&raw_code, &key_down)) {
      kbdDelay(kPollDelayLoops);
      continue;
    }
    if (!key_down) {
      continue;
    }

    (void)raw_code;
    return;
  }
}

unsigned char kbd_wait_for_key_or_esc(void) {
  for (;;) {
    unsigned char raw_code;
    unsigned char key_down;

#if defined(_DEBUG) && (_DEBUG > 0)
    if (kbdSerialPoll(&raw_code)) {
      return (raw_code == kSerialEsc) ? KEY_ESC : 0U;
    }
#endif

    if (!kbdPollRawEvent(&raw_code, &key_down)) {
      kbdDelay(kPollDelayLoops);
      continue;
    }
    if (!key_down) {
      continue;
    }

    if (raw_code == kRawEsc) {
      return KEY_ESC;
    }
    return 0U;
  }
}
