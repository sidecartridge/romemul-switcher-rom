/**
 * File: src/common/selftest.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The device self-test screen.
 */

#ifndef SELFTEST_H_
#define SELFTEST_H_

/* What the self-test needs from the chooser (EPIC-03). */
typedef struct {
  unsigned long rom_base;        /* where the device serves the image */
  unsigned long rom_size;        /* the image size: 192, 256 or 512 KB */
  const unsigned char *params;   /* the parameters page the chooser loaded */
  const unsigned char *catalog;  /* the catalog the chooser loaded */
  int entries;                   /* catalog entries */
  unsigned char default_color;
} selftest_env_t;

/* Runs every test, shows the results, and returns when the user presses ESC. */
void selftest_run(const selftest_env_t *env);

#endif /* SELFTEST_H_ */
