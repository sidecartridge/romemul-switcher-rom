#include "kbd.h"

enum {
  kAciaBaseAddr = 0x00FFFC00UL,
  kIkbdAciaCtrlMasterReset = 0x03U,
  kIkbdAciaCtrlPolling = 0x16U,
  kIkbdStatusRxFull = 0x01U,
  kIkbdStatusTxEmpty = 0x02U,
  kIkbdStatusErrMask = 0x70U,
  kIkbdCmdTurnMouseOff = 0x12U,
  kInvalidScancode = 0xFFU,
  kKeyReleaseMask = 0x80U,
  kPollDelayLoops = 2500U,
  kPacketHeaderMouseRelativeMin = 0xF8U,
  kPacketHeaderMouseRelativeMax = 0xFBU,
  kPacketHeaderMouseAbsolute = 0xF7U,
  kPacketHeaderTimeOfDay = 0xFCU,
  kPacketHeaderJoystickEventA = 0xFEU,
  kPacketHeaderJoystickEventB = 0xFFU,
  kPacketHeaderStatusReport = 0xF6U,
  kPacketHeaderJoystickStatus = 0xFDU,
  kDropCountMouseRelative = 2U,
  kDropCountMouseAbsolute = 4U,
  kDropCountTimeOfDay = 6U,
  kDropCountJoystickEvent = 1U
};

struct st_acia_interface {
  volatile unsigned char control; /* read=status, write=control */
  volatile unsigned char dummy1;
  volatile unsigned char data;
  volatile unsigned char dummy2;
};

#define ST_IKBD_ACIA \
  (*(volatile struct st_acia_interface *)(unsigned long)kAciaBaseAddr)

static unsigned char kbdAciaInitialized = 0U;
static unsigned char kbdDropBytes = 0U;

static void kbdWriteByte(unsigned char value) {
  while (!(ST_IKBD_ACIA.control & kIkbdStatusTxEmpty)) {
    /* wait for TX data register empty */
  }
  ST_IKBD_ACIA.data = value;
}

static void kbdAciaInit(void) {
  if (kbdAciaInitialized) {
    return;
  }

  /* Configure MC6850 for polled receive: reset, then /64 + 8N1 + RX IRQ off. */
  ST_IKBD_ACIA.control = kIkbdAciaCtrlMasterReset;
  (void)ST_IKBD_ACIA.control;
  ST_IKBD_ACIA.control = kIkbdAciaCtrlPolling;
  (void)ST_IKBD_ACIA.control;

  /* Keyboard-only runtime: turn mouse reporting off at IKBD source. */
  kbdWriteByte(kIkbdCmdTurnMouseOff);

  /* Drain any stale byte already present in RDR. */
  while (ST_IKBD_ACIA.control & kIkbdStatusRxFull) {
    (void)ST_IKBD_ACIA.data;
  }

  kbdDropBytes = 0U;
  kbdAciaInitialized = 1U;
}

static void kbdDelay(unsigned long loops) {
  while (loops--) {
    __asm__ volatile("nop\n nop\n" ::: "memory");
  }
}

unsigned char kbd_poll_scancode(void) {
  kbdAciaInit();

  for (;;) {
    unsigned char status;
    unsigned char data;

    /*
     * MC6850 access sequence: read STATUS first, then DATA when RDRF=1.
     * This also allows checking framing/overrun/parity flags from STATUS.
     */
    status = ST_IKBD_ACIA.control;
    if (!(status & kIkbdStatusRxFull)) {
      return kInvalidScancode;
    }

    data = ST_IKBD_ACIA.data;
    if (status & kIkbdStatusErrMask) {
      continue;
    }

    /* Consume pending bytes from a previously identified non-keyboard packet.
     */
    if (kbdDropBytes) {
      kbdDropBytes--;
      continue;
    }

    /*
     * Filter IKBD packets that are not keyboard scancodes.
     * Keep only keyboard make/break stream and ignore mouse/joystick/report
     * frames.
     */
    if (data >= kPacketHeaderMouseRelativeMin &&
        data <= kPacketHeaderMouseRelativeMax) {
      kbdDropBytes = kDropCountMouseRelative;
      continue;
    }
    if (data == kPacketHeaderMouseAbsolute) {
      kbdDropBytes = kDropCountMouseAbsolute;
      continue;
    }
    if (data == kPacketHeaderTimeOfDay) {
      kbdDropBytes = kDropCountTimeOfDay;
      continue;
    }
    if (data == kPacketHeaderJoystickEventA ||
        data == kPacketHeaderJoystickEventB) {
      kbdDropBytes = kDropCountJoystickEvent;
      continue;
    }
    if (data == kPacketHeaderStatusReport ||
        data == kPacketHeaderJoystickStatus) {
      continue;
    }

    return data;
  }
}

unsigned char kbd_poll_scancode_wait(void) {
  for (;;) {
    unsigned char scan = kbd_poll_scancode();
    if (scan == kInvalidScancode) {
      kbdDelay(kPollDelayLoops);
      continue;
    }
    if (scan & kKeyReleaseMask) {
      continue;
    }
    return scan;
  }
}

void kbd_wait_for_key_press(void) {
  (void)kbd_poll_scancode_wait();
}
