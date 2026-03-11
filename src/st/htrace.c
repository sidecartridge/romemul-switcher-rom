/**
 * File: src/st/htrace.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST Hatari trace helper implementation.
 */

#include "htrace.h"

#if defined(_DEBUG) && (_DEBUG > 0)

extern long nf_get_id(const char *feature_name);
extern long nf_call(long id, ...);

enum { kInvalidNatFeatId = 0L };

static long gNfStderrId = kInvalidNatFeatId;

void hatari_trace_init(void) {
  static const char kFeatureName[] = "NF_STDERR";
  gNfStderrId = nf_get_id(kFeatureName);
  if (gNfStderrId != kInvalidNatFeatId) {
    hatari_trace_msg("[romswdbg] trace init\n");
  }
}

void hatari_trace_msg(const char *message) {
  if (gNfStderrId == kInvalidNatFeatId || message == (const char *)0) {
    return;
  }
  (void)nf_call(gNfStderrId, message);
}

#else

void hatari_trace_init(void) {}

void hatari_trace_msg(const char *message) { (void)message; }

#endif
