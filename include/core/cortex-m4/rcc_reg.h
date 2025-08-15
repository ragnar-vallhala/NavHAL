#ifndef CORTEX_M4_RCC_REG_H
#define CORTEX_M4_RCC_REG_H
#include "common/hal_types.h"
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

#define RCC_ON 0x1
#define RCC_OFF 0x0

// RCC_CR register bit positions
#define RCC_CR_HSE_ON_BIT 16    ///< HSE clock enable bit
#define RCC_CR_HSE_READY_BIT 17 ///< HSE clock ready flag bit
#define RCC_CR_HSI_ON_BIT 0     ///< HSI clock enable bit
#define RCC_CR_HSI_READY_BIT 1  ///< HSI clock ready flag bit
#define RCC_CR_PLL_ON_BIT 24    ///< PLL enable bit
#define RCC_CR_PLL_READY_BIT 25 ///< PLL ready flag bit

// RCC_CR masks
#define RCC_CR_HSEON (1 << RCC_CR_HSE_ON_BIT) ///< Enable external clock
#define RCC_CR_HSERDY                                                          \
  (1 << RCC_CR_HSE_READY_BIT) ///< Check external clock is ready
#define RCC_CR_HSION (1 << RCC_CR_HSI_ON_BIT)
#define RCC_CR_HSIRDY (1 << RCC_CR_HSI_READY_BIT)
#define RCC_CR_PLLON (1 << RCC_CR_PLL_ON_BIT)
#define RCC_CR_PLLRDY (1 << RCC_CR_PLL_READY_BIT)

// RCC_PLLCFGR register bit positions
#define RCC_PLLCFGR_SRC_BIT 22  ///< PLL source selection bit
#define RCC_PLLCFGR_PLLM_BIT 0  ///< PLLM bits (6 bits)
#define RCC_PLLCFGR_PLLN_BIT 6  ///< PLLN bits (9 bits)
#define RCC_PLLCFGR_PLLP_BIT 16 ///< PLLP bits (2 bits)
#define RCC_PLLCFGR_PLLQ_BIT 24 ///< PLLQ bits (4 bits)

// RCC_PLL_CFGR masks
#define RCC_PLLCFGR_SRC (1 << RCC_PLLCFGR_SRC_BIT) // 0 for hsi and 1 for hse
#define RCC_PLLCFGR_PLLM(m) (((m) & 0x3F) << RCC_PLLCFGR_PLLM_BIT)
#define RCC_PLLCFGR_PLLN(n) (((n) & 0x1FF) << RCC_PLLCFGR_PLLN_BIT)
#define RCC_PLLCFGR_PLLP(p) ((((p) >> 1) - 1) << RCC_PLLCFGR_PLLP_BIT)
#define RCC_PLLCFGR_PLLQ(q) (((q) & 0x0F) << RCC_PLLCFGR_PLLQ_BIT)

// RCC_CFGR register bit positions
#define RCC_CFGR_HPRE_BIT 4   ///< AHB prescaler bits (4 bits)
#define RCC_CFGR_PPRE1_BIT 10 ///< APB1 prescaler bits (3 bits)
#define RCC_CFGR_PPRE2_BIT 13 ///< APB2 prescaler bits (3 bits)
#define RCC_CFGR_SW_BIT 0     ///< System clock switch bits (2 bits)
#define RCC_CFGR_SWS_BIT 2    ///< System clock switch status bits (2 bits)

// RCC_CFGR masks
#define RCC_CFGR_HPRE_MASK (0x3 << RCC_CFGR_HPRE_BIT)
#define RCC_CFGR_PPRE1_MASK (0x7 << RCC_CFGR_PPRE1_BIT)
#define RCC_CFGR_PPRE2_MASK (0x7 << RCC_CFGR_PPRE2_BIT)

// Prescaler values
// AHB prescaler (HPRE) division factors encoded in RCC_CFGR bits 7:4
typedef enum {
  RCC_CFGR_HPRE_DIV1 = 0x0,   // SYSCLK not divided
  RCC_CFGR_HPRE_DIV2 = 0x8,   // SYSCLK divided by 2
  RCC_CFGR_HPRE_DIV4 = 0x9,   // SYSCLK divided by 4
  RCC_CFGR_HPRE_DIV8 = 0xA,   // SYSCLK divided by 8
  RCC_CFGR_HPRE_DIV16 = 0xB,  // SYSCLK divided by 16
  RCC_CFGR_HPRE_DIV64 = 0xC,  // SYSCLK divided by 64
  RCC_CFGR_HPRE_DIV128 = 0xD, // SYSCLK divided by 128
  RCC_CFGR_HPRE_DIV256 = 0xE, // SYSCLK divided by 256
  RCC_CFGR_HPRE_DIV512 = 0xF  // SYSCLK divided by 512
} rcc_cfgr_hpre_div_t;

// APB1 prescaler (PPRE1) division factors encoded in RCC_CFGR bits 12:10
typedef enum {
  RCC_CFGR_PPRE_DIV1 = 0x0, // HCLK not divided
  RCC_CFGR_PPRE_DIV2 = 0x4, // HCLK divided by 2
  RCC_CFGR_PPRE_DIV4 = 0x5, // HCLK divided by 4
  RCC_CFGR_PPRE_DIV8 = 0x6, // HCLK divided by 8
  RCC_CFGR_PPRE_DIV16 = 0x7 // HCLK divided by 16
} rcc_cfgr_ppre_div_t;

#define RCC_CFGR_HPRE_DIV(n) (n << RCC_CFGR_HPRE_BIT)
#define RCC_CFGR_PPRE1_DIV(n) (n << RCC_CFGR_PPRE1_BIT)
#define RCC_CFGR_PPRE2_DIV(n) (n << RCC_CFGR_PPRE2_BIT)

#define RCC_CFGR_HPRE_DIV1 0x0  ///< AHB clock: SYSCLK not divided
#define RCC_CFGR_PPRE1_DIV4 0x5 ///< APB1 clock: AHB divided by 4
#define RCC_CFGR_PPRE2_DIV2 0x4 ///< APB2 clock: AHB divided by 2


#endif // !CORTEX_M4_RCC_REG_H
