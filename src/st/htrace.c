#include "htrace.h"

#if defined(_DEBUG) && (_DEBUG > 0)

extern long nf_get_id(const char *feature_name);
extern long nf_call(long id, ...);

enum { kInvalidNatFeatId = 0L };

static long gNfStderrId = kInvalidNatFeatId;
static char gKeyEventLine[] = "[romswdbg] sc=0x00 map=0 prev=0\n";

static char to_hex_nibble(unsigned char value) {
  if (value < 10U) {
    return (char)('0' + (char)value);
  }
  return (char)('A' + (char)(value - 10U));
}

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

void hatari_trace_key_event(unsigned char scanCode, unsigned char mappedBank,
                            unsigned char previousBank) {
  gKeyEventLine[17] = to_hex_nibble((unsigned char)((scanCode >> 4) & 0x0FU));
  gKeyEventLine[18] = to_hex_nibble((unsigned char)(scanCode & 0x0FU));
  gKeyEventLine[25] = (char)('0' + (char)mappedBank);
  gKeyEventLine[32] = (char)('0' + (char)previousBank);
  hatari_trace_msg(gKeyEventLine);
}

#else

void hatari_trace_init(void) {}

void hatari_trace_msg(const char *message) { (void)message; }

void hatari_trace_key_event(unsigned char scanCode, unsigned char mappedBank,
                            unsigned char previousBank) {
  (void)scanCode;
  (void)mappedBank;
  (void)previousBank;
}

#endif
