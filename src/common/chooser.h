/**
 * File: src/common/chooser.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared ROM chooser declarations.
 */

#ifndef CHOOSER_H_
#define CHOOSER_H_

#include "commands.h"

void chooser_loop(unsigned long rom_base_addr, helper_trace_fn_t trace_fn,
                  unsigned char default_color, const char *computer_model);

#endif /* CHOOSER_H_ */
