/**
 * File: src/common/screenshot.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The screenshot build: a cartridge that always answers and a
 *              fixed key script.
 */

#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

/*
 * The screenshot build (tools/dev/build.sh <platform> screenshot) shows the
 * screens of a v4.0.0 board in Hatari and FS-UAE, for the user documentation.
 * It is a release build in which these two stand in for the cartridge and the
 * keyboard. Never for hardware.
 */

/* The page a v4.0.0 board's flash holds at offset, as read_flash_page()
   leaves it: the parameters page, or a page of this image's ROM list.
   Always 0: the read never fails. */
int screenshot_read_flash_page(unsigned char *page, unsigned long offset);

/* The next key of the script, once the screen has waited 5 s for it. After
   the last key, it never returns. */
unsigned char screenshot_key(void);

#endif /* SCREENSHOT_H_ */
