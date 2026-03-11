/**
 * File: src/amiga/hw.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga hardware register definitions.
 */

#pragma once

#define AMIGA_CUSTOM_BASE_ADDR_UL 0x00DFF000UL

#define AMIGA_REG16(offset) \
  (*(volatile unsigned short *)(unsigned long)(AMIGA_CUSTOM_BASE_ADDR_UL + (offset)))

#define AMIGA_COP1LCH AMIGA_REG16(0x080U)
#define AMIGA_COP1LCL AMIGA_REG16(0x082U)
#define AMIGA_DMACON AMIGA_REG16(0x096U)
#define AMIGA_INTENA AMIGA_REG16(0x09AU)
#define AMIGA_INTREQ AMIGA_REG16(0x09CU)
#define AMIGA_ADKCON AMIGA_REG16(0x09EU)
#define AMIGA_BPLCON0 AMIGA_REG16(0x100U)
#define AMIGA_BPLCON1 AMIGA_REG16(0x102U)
#define AMIGA_BPLCON2 AMIGA_REG16(0x104U)
#define AMIGA_BPLCON3 AMIGA_REG16(0x10CU)
#define AMIGA_BPL1MOD AMIGA_REG16(0x108U)
#define AMIGA_BPL2MOD AMIGA_REG16(0x10AU)
#define AMIGA_COLOR00 AMIGA_REG16(0x180U)
#define AMIGA_COLOR01 AMIGA_REG16(0x182U)
#define AMIGA_COLOR02 AMIGA_REG16(0x184U)
#define AMIGA_COLOR03 AMIGA_REG16(0x186U)

#define AMIGA_CIAA_SDR (*(volatile unsigned char *)(unsigned long)0x00BFEC01UL)
#define AMIGA_CIAA_ICR (*(volatile unsigned char *)(unsigned long)0x00BFED01UL)
#define AMIGA_CIAA_CRA (*(volatile unsigned char *)(unsigned long)0x00BFEE01UL)
