/**
 * File: src/common/test.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Embedded test data declarations.
 */

#ifndef TEST_H_
#define TEST_H_

#if (defined(_DEBUG) && (_DEBUG > 0)) || (defined(_TEST) && (_TEST > 0)) || \
    (defined(TEST) && (TEST > 0))

enum {
  kFlashParamsRawSize = 4096U,
  /* The 33 records the test catalog holds, then the zero record that ends
     the chooser's walk: the test image must stay under 64 KB for the
     address-line layout (EPIC-03 STORY-03). */
  kFlashCatalogRawSize = 8704U
};

/* The fake flash is read-only and stays in ROM: as RAM data it would not
   fit the ST's RAM area (EPIC-00 STORY-02). The a.out objects of this
   toolchain have no named sections, so const data lands in test.c.o's .text,
   which both linker scripts place in ROM by file name (.romdata). */
extern const unsigned char flashParamsRaw[kFlashParamsRawSize];
extern const unsigned char flashCatalogRaw[kFlashCatalogRawSize];

#endif /* _DEBUG || _TEST || TEST */

#endif /* TEST_H_ */
