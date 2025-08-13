#ifndef CORTEX_M4_RCC_REG_H
#define CORTEX_M4_RCC_REG_H

/**
 * @file rcc_reg.h
 * @brief Register map for Reset and Clock Control (RCC) peripheral on STM32F4 series.
 * @details
 * This header defines the memory-mapped structure for the RCC peripheral,
 * located at base address `0x40023800` on STM32F4 microcontrollers.
 *
 * Each member corresponds to a 32-bit hardware register controlling clock
 * sources, prescalers, peripheral resets, and clock enables.
 *
 * Reserved fields are placeholders to maintain correct register offsets
 * and must not be modified.
 */

#include "utils/types.h"
#include <stdint.h>

/**
 * @brief RCC register map structure.
 * @note Access via the `RCC` macro to interact with hardware registers.
 */
typedef struct {
  __IO uint32_t CR;          /**< 0x00: Clock control register. */
  __IO uint32_t PLLCFGR;     /**< 0x04: PLL configuration register. */
  __IO uint32_t CFGR;        /**< 0x08: Clock configuration register. */
  __IO uint32_t CIR;         /**< 0x0C: Clock interrupt register. */
  __IO uint32_t AHB1RSTR;    /**< 0x10: AHB1 peripheral reset register. */
  __IO uint32_t AHB2RSTR;    /**< 0x14: AHB2 peripheral reset register. */
  __IO uint32_t RESERVED_18; /**< 0x18: Reserved. */
  __IO uint32_t RESERVED_1C; /**< 0x1C: Reserved. */
  __IO uint32_t APB1RSTR;    /**< 0x20: APB1 peripheral reset register. */
  __IO uint32_t APB2RSTR;    /**< 0x24: APB2 peripheral reset register. */
  __IO uint32_t RESERVED_28; /**< 0x28: Reserved. */
  __IO uint32_t RESERVED_2C; /**< 0x2C: Reserved. */
  __IO uint32_t AHB1ENR;     /**< 0x30: AHB1 peripheral clock enable register. */
  __IO uint32_t AHB2ENR;     /**< 0x34: AHB2 peripheral clock enable register. */
  __IO uint32_t RESERVED_38; /**< 0x38: Reserved. */
  __IO uint32_t RESERVED_3C; /**< 0x3C: Reserved. */
  __IO uint32_t APB1ENR;     /**< 0x40: APB1 peripheral clock enable register. */
  __IO uint32_t APB2ENR;     /**< 0x44: APB2 peripheral clock enable register. */
  __IO uint32_t RESERVED_48; /**< 0x48: Reserved. */
  __IO uint32_t RESERVED_4C; /**< 0x4C: Reserved. */
  __IO uint32_t AHB1LPENR;   /**< 0x50: AHB1 peripheral clock enable in low power mode register. */
  __IO uint32_t AHB2LPENR;   /**< 0x54: AHB2 peripheral clock enable in low power mode register. */
  __IO uint32_t RESERVED_58; /**< 0x58: Reserved. */
  __IO uint32_t RESERVED_5C; /**< 0x5C: Reserved. */
  __IO uint32_t APB1LPENR;   /**< 0x60: APB1 peripheral clock enable in low power mode register. */
  __IO uint32_t APB2LPENR;   /**< 0x64: APB2 peripheral clock enable in low power mode register. */
  __IO uint32_t RESERVED_68; /**< 0x68: Reserved. */
  __IO uint32_t RESERVED_6C; /**< 0x6C: Reserved. */
  __IO uint32_t BDCR;        /**< 0x70: Backup domain control register. */
  __IO uint32_t CSR;         /**< 0x74: Clock control & status register. */
  __IO uint32_t RESERVED_78; /**< 0x78: Reserved. */
  __IO uint32_t RESERVED_7C; /**< 0x7C: Reserved. */
  __IO uint32_t SSCGR;       /**< 0x80: Spread spectrum clock generation register. */
  __IO uint32_t PLLI2SCFGR;  /**< 0x84: PLLI2S configuration register. */
  __IO uint32_t RESERVED_88; /**< 0x88: Reserved. */
  __IO uint32_t DCKCFGR;     /**< 0x8C: Dedicated clock configuration register. */
} RCC_Typedef;

/** @brief Base address of RCC peripheral. */
#define RCC_BASE_ADDR 0x40023800

/**
 * @brief Pointer to RCC peripheral registers.
 * @details Example usage:
 * @code
 * // Enable GPIOA clock
 * RCC->AHB1ENR |= (1 << 0);
 * @endcode
 */
#define RCC ((RCC_Typedef *)RCC_BASE_ADDR)

#endif // !CORTEX_M4_RCC_REG_H
