/**
 * File: src/common/nonce.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The nonce of each command frame.
 */

#include "nonce.h"

#include "platform.h"
#include "trace.h"

static const unsigned long kNonceMask = 0xFFFEFFFEUL;
/* A xorshift state must not be zero. */
static const unsigned long kNonceSeedFallback = 0x9E3779B9UL;

static unsigned long gNonceState = 0UL;
static unsigned long gNonceLast = 0UL;

static unsigned long nonceStep(unsigned long state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

unsigned long nonce_next(unsigned long rom_base_address) {
  const unsigned long current =
      *(const volatile unsigned long *)rom_base_address;
  unsigned long nonce;

  /* Seeded once: in ROM mode the platform seed is the same for a whole run,
     so the state, not the seed, must change from frame to frame. */
  if (gNonceState == 0UL) {
    gNonceState = platform_get_system_time_seed();
    if (gNonceState == 0UL) {
      gNonceState = kNonceSeedFallback;
    }
  }
  do {
    gNonceState = nonceStep(gNonceState);
    nonce = gNonceState & kNonceMask;
  } while (nonce == gNonceLast || nonce == current);
  gNonceLast = nonce;
  TRACE("nonce %08lx", nonce);
  return nonce;
}
