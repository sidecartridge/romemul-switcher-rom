/**
 * File: src/common/nonce.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The nonce of each command frame.
 */

#ifndef NONCE_H_
#define NONCE_H_

/*
 * The nonce for the next command frame (EPIC-03 STORY-01): masked to even
 * words as the frame needs, and never equal to the previous nonce or to the
 * long now at rom_base_address, so a stale echo there cannot pass for the
 * firmware's answer. The firmware itself never checks it.
 */
unsigned long nonce_next(unsigned long rom_base_address);

#endif /* NONCE_H_ */
