#ifndef CORTEX_M4_RCC_REG_H
#define CORTEX_M4_RCC_REG_H
#include "utils/types.h"
#include <stdint.h>

typedef struct {
  __IO uint32_t CR;          // 0x00
  __IO uint32_t PLLCFGR;     // 0x04
  __IO uint32_t CFGR;        // 0x08
  __IO uint32_t CIR;         // 0x0C
  __IO uint32_t AHB1RSTR;    // 0x10
  __IO uint32_t AHB2RSTR;    // 0x14
  __IO uint32_t RESERVED_18; // 0x18
  __IO uint32_t RESERVED_1C; // 0x1C
  __IO uint32_t APB1RSTR;    // 0x20
  __IO uint32_t APB2RSTR;    // 0x24
  __IO uint32_t RESERVED_28; // 0x28
  __IO uint32_t RESERVED_2C; // 0x2C
  __IO uint32_t AHB1ENR;     // 0x30
  __IO uint32_t AHB2ENR;     // 0x34
  __IO uint32_t RESERVED_38; // 0x38
  __IO uint32_t RESERVED_3C; // 0x3C
  __IO uint32_t APB1ENR;     // 0x40
  __IO uint32_t APB2ENR;     // 0x44
  __IO uint32_t RESERVED_48; // 0x48
  __IO uint32_t RESERVED_4C; // 0x4C
  __IO uint32_t AHB1LPENR;   // 0x50
  __IO uint32_t AHB2LPENR;   // 0x54
  __IO uint32_t RESERVED_58; // 0x58
  __IO uint32_t RESERVED_5C; // 0x5C
  __IO uint32_t APB1LPENR;   // 0x60
  __IO uint32_t APB2LPENR;   // 0x64
  __IO uint32_t RESERVED_68; // 0x68
  __IO uint32_t RESERVED_6C; // 0x6C
  __IO uint32_t BDCR;        // 0x70
  __IO uint32_t CSR;         // 0x74
  __IO uint32_t RESERVED_78; // 0x78
  __IO uint32_t RESERVED_7C; // 0x7C
  __IO uint32_t SSCGR;       // 0x80
  __IO uint32_t PLLI2SCFGR;  // 0x84
  __IO uint32_t RESERVED_88; // 0x88
  __IO uint32_t DCKCFGR;     // 0x8C
} RCC_Typedef;
#define RCC_BASE_ADDR 0x40023800
#define RCC ((RCC_Typedef *)RCC_BASE_ADDR)
#endif // !CORTEX_M4_RCC_REG_H
