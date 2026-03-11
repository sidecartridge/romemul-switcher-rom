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
  kFlashCatalogRawSize = 61440U,
  kFlashStorageRawSize = 65536U
};

extern unsigned char flashParamsRaw[kFlashParamsRawSize];
extern unsigned char flashCatalogRaw[kFlashCatalogRawSize];
extern unsigned char flashStorageRaw[kFlashStorageRawSize];

#endif /* _DEBUG || _TEST || TEST */

#endif /* TEST_H_ */
