#pragma once


void pa_init(unsigned long base, unsigned long size);
void *pa_alloc(unsigned long size);
void pa_free(void *ptr);
void *pa_alloc_aligned(unsigned long size, unsigned long align);
