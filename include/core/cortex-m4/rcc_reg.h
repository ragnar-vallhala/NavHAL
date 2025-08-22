/**
 * @file cortex_m4_rcc_reg.h
 * @brief HAL interface for RCC (Reset and Clock Control) registers on STM32F4.
 *
 * @details
 * Provides:
 * - RCC register structure mapping.
 * - Base address and pointer to RCC.
 * - Bit positions, masks, and prescaler enums for AHB/APB buses and PLL.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_RCC_REG_H
#define CORTEX_M4_RCC_REG_H

#include "common/hal_types.h"
#include <stdint.h>

/**
 * @brief RCC register map structure.
 */
typedef struct {
  __IO uint32_t CR;          /**< Clock control register */
  __IO uint32_t PLLCFGR;     /**< PLL configuration register */
  __IO uint32_t CFGR;        /**< Clock configuration register */
  __IO uint32_t CIR;         /**< Clock interrupt register */
  __IO uint32_t AHB1RSTR;    /**< AHB1 peripheral reset register */
  __IO uint32_t AHB2RSTR;    /**< AHB2 peripheral reset register */
  __IO uint32_t RESERVED_18; /**< Reserved */
  __IO uint32_t RESERVED_1C; /**< Reserved */
  __IO uint32_t APB1RSTR;    /**< APB1 peripheral reset register */
  __IO uint32_t APB2RSTR;    /**< APB2 peripheral reset register */
  __IO uint32_t RESERVED_28; /**< Reserved */
  __IO uint32_t RESERVED_2C; /**< Reserved */
  __IO uint32_t AHB1ENR;     /**< AHB1 peripheral clock enable register */
  __IO uint32_t AHB2ENR;     /**< AHB2 peripheral clock enable register */
  __IO uint32_t RESERVED_38; /**< Reserved */
  __IO uint32_t RESERVED_3C; /**< Reserved */
  __IO uint32_t APB1ENR;     /**< APB1 peripheral clock enable register */
  __IO uint32_t APB2ENR;     /**< APB2 peripheral clock enable register */
  __IO uint32_t RESERVED_48; /**< Reserved */
  __IO uint32_t RESERVED_4C; /**< Reserved */
  __IO uint32_t AHB1LPENR;   /**< AHB1 clock enable in low power mode */
  __IO uint32_t AHB2LPENR;   /**< AHB2 clock enable in low power mode */
  __IO uint32_t RESERVED_58; /**< Reserved */
  __IO uint32_t RESERVED_5C; /**< Reserved */
  __IO uint32_t APB1LPENR;   /**< APB1 clock enable in low power mode */
  __IO uint32_t APB2LPENR;   /**< APB2 clock enable in low power mode */
  __IO uint32_t RESERVED_68; /**< Reserved */
  __IO uint32_t RESERVED_6C; /**< Reserved */
  __IO uint32_t BDCR;        /**< Backup domain control register */
  __IO uint32_t CSR;         /**< Clock control & status register */
  __IO uint32_t RESERVED_78; /**< Reserved */
  __IO uint32_t RESERVED_7C; /**< Reserved */
  __IO uint32_t SSCGR;       /**< Spread spectrum clock generation register */
  __IO uint32_t PLLI2SCFGR;  /**< PLLI2S configuration register */
  __IO uint32_t RESERVED_88; /**< Reserved */
  __IO uint32_t DCKCFGR;     /**< Dedicated clock configuration register */
} RCC_Typedef;

/** @brief RCC base address */
#define RCC_BASE_ADDR 0x40023800

/** @brief Pointer to RCC peripheral registers */
#define RCC ((RCC_Typedef *)RCC_BASE_ADDR)

/** @brief Enable flag */
#define RCC_ON 0x1
/** @brief Disable flag */
#define RCC_OFF 0x0

/* RCC_CR bit positions */
#define RCC_CR_HSE_ON_BIT 16    /**< HSE clock enable bit */
#define RCC_CR_HSE_READY_BIT 17 /**< HSE clock ready flag */
#define RCC_CR_HSI_ON_BIT 0     /**< HSI clock enable bit */
#define RCC_CR_HSI_READY_BIT 1  /**< HSI clock ready flag */
#define RCC_CR_PLL_ON_BIT 24    /**< PLL enable bit */
#define RCC_CR_PLL_READY_BIT 25 /**< PLL ready flag */

/* RCC_CR masks */
#define RCC_CR_HSEON (1 << RCC_CR_HSE_ON_BIT)   /**< Enable external HSE clock */
#define RCC_CR_HSERDY (1 << RCC_CR_HSE_READY_BIT) /**< External HSE clock ready flag */
#define RCC_CR_HSION (1 << RCC_CR_HSI_ON_BIT)   /**< Enable internal HSI clock */
#define RCC_CR_HSIRDY (1 << RCC_CR_HSI_READY_BIT) /**< Internal HSI clock ready flag */
#define RCC_CR_PLLON (1 << RCC_CR_PLL_ON_BIT)   /**< Enable PLL */
#define RCC_CR_PLLRDY (1 << RCC_CR_PLL_READY_BIT) /**< PLL ready flag */

/* RCC_PLLCFGR bit positions */
#define RCC_PLLCFGR_SRC_BIT 22  /**< PLL source selection bit */
#define RCC_PLLCFGR_PLLM_BIT 0  /**< PLLM bits */
#define RCC_PLLCFGR_PLLN_BIT 6  /**< PLLN bits */
#define RCC_PLLCFGR_PLLP_BIT 16 /**< PLLP bits */
#define RCC_PLLCFGR_PLLQ_BIT 24 /**< PLLQ bits */

/* RCC_PLLCFGR masks */
#define RCC_PLLCFGR_SRC (1 << RCC_PLLCFGR_SRC_BIT) /**< 0: HSI, 1: HSE */
#define RCC_PLLCFGR_PLLM(m) (((m) & 0x3F) << RCC_PLLCFGR_PLLM_BIT) /**< Set PLLM */
#define RCC_PLLCFGR_PLLN(n) (((n) & 0x1FF) << RCC_PLLCFGR_PLLN_BIT) /**< Set PLLN */
#define RCC_PLLCFGR_PLLP(p) ((((p) >> 1) - 1) << RCC_PLLCFGR_PLLP_BIT) /**< Set PLLP */
#define RCC_PLLCFGR_PLLQ(q) (((q) & 0x0F) << RCC_PLLCFGR_PLLQ_BIT) /**< Set PLLQ */

/* RCC_CFGR bit positions */
#define RCC_CFGR_HPRE_BIT 4   /**< AHB prescaler bits */
#define RCC_CFGR_PPRE1_BIT 10 /**< APB1 prescaler bits */
#define RCC_CFGR_PPRE2_BIT 13 /**< APB2 prescaler bits */
#define RCC_CFGR_SW_BIT 0     /**< System clock switch bits */
#define RCC_CFGR_SWS_BIT 2    /**< System clock switch status bits */

/* RCC_CFGR masks */
#define RCC_CFGR_HPRE_MASK (0xF << RCC_CFGR_HPRE_BIT) /**< Mask for AHB prescaler */
#define RCC_CFGR_PPRE1_MASK (0x7 << RCC_CFGR_PPRE1_BIT) /**< Mask for APB1 prescaler */
#define RCC_CFGR_PPRE2_MASK (0x7 << RCC_CFGR_PPRE2_BIT) /**< Mask for APB2 prescaler */

/**
 * @brief AHB prescaler division factors.
 */
typedef enum {
  RCC_CFGR_HPRE_DIV1 = 0x0,   /**< SYSCLK not divided */
  RCC_CFGR_HPRE_DIV2 = 0x8,   /**< SYSCLK divided by 2 */
  RCC_CFGR_HPRE_DIV4 = 0x9,   /**< SYSCLK divided by 4 */
  RCC_CFGR_HPRE_DIV8 = 0xA,   /**< SYSCLK divided by 8 */
  RCC_CFGR_HPRE_DIV16 = 0xB,  /**< SYSCLK divided by 16 */
  RCC_CFGR_HPRE_DIV64 = 0xC,  /**< SYSCLK divided by 64 */
  RCC_CFGR_HPRE_DIV128 = 0xD, /**< SYSCLK divided by 128 */
  RCC_CFGR_HPRE_DIV256 = 0xE, /**< SYSCLK divided by 256 */
  RCC_CFGR_HPRE_DIV512 = 0xF  /**< SYSCLK divided by 512 */
} rcc_cfgr_hpre_div_t;

/**
 * @brief APB prescaler division factors.
 */
typedef enum {
  RCC_CFGR_PPRE_DIV1 = 0x0,  /**< HCLK not divided */
  RCC_CFGR_PPRE_DIV2 = 0x4,  /**< HCLK divided by 2 */
  RCC_CFGR_PPRE_DIV4 = 0x5,  /**< HCLK divided by 4 */
  RCC_CFGR_PPRE_DIV8 = 0x6,  /**< HCLK divided by 8 */
  RCC_CFGR_PPRE_DIV16 = 0x7  /**< HCLK divided by 16 */
} rcc_cfgr_ppre_div_t;

#define RCC_CFGR_HPRE_DIV(n) (n << RCC_CFGR_HPRE_BIT)   /**< Set AHB prescaler */
#define RCC_CFGR_PPRE1_DIV(n) (n << RCC_CFGR_PPRE1_BIT) /**< Set APB1 prescaler */
#define RCC_CFGR_PPRE2_DIV(n) (n << RCC_CFGR_PPRE2_BIT) /**< Set APB2 prescaler */



#endif // !CORTEX_M4_RCC_REG_H
