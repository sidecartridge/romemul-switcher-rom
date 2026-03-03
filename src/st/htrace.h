#pragma once

void hatari_trace_init(void);
void hatari_trace_msg(const char *message);
void hatari_trace_key_event(unsigned char scanCode, unsigned char mappedBank,
                            unsigned char previousBank);
