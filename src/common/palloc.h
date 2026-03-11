/**
 * File: src/common/palloc.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared bump allocator declarations.
 */

#pragma once


void pa_init(unsigned long base, unsigned long size);
void *pa_alloc(unsigned long size);
void pa_free(void *ptr);
void *pa_alloc_aligned(unsigned long size, unsigned long align);
