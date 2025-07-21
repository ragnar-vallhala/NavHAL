#ifndef CORTEX_M4_CLOCK_H
#define CORTEX_M4_CLOCK_H
#include "utils/types.h"

typedef struct
{
    hal_clock_source_t input_src;
    uint8_t pll_m;
    uint16_t pll_n;
    uint8_t pll_p;
    uint8_t pll_q;
} hal_pll_config_t;

#define RCC 0x40023800
#define RCC_CR_OFFSET 0x00
#define RCC_CR_HSE_ON_BIT 16
#define RCC_CR_HSE_READY_BIT 17

#define RCC_CR_HSI_ON_BIT 0
#define RCC_CR_HSI_READY_BIT 1
#define RCC_CR_PLL_ON_BIT 24
#define RCC_CR_PLL_READY_BIT 25

#define RCC_PLLCFGR_OFFSET 0x04
#define RCC_PLLCFGR_SRC_BIT 22
#define RCC_PLLCFGR_PLLM_BIT 0
#define RCC_PLLCFGR_PLLN_BIT 6
#define RCC_PLLCFGR_PLLP_BIT 16
#define RCC_PLLCFGR_PLLQ_BIT 24

#define RCC_CFGR_OFFSET 0x08
#define RCC_CFGR_HPRE_BIT 4   // Set and cleared by software to control AHB clock division factor.
#define RCC_CFGR_PPRE1_BIT 10 // Set and cleared by software to control APB low-speed clock division factor
#define RCC_CFGR_PPRE2_BIT 13 // Set and cleared by software to control APB high-speed clock division factor

#define RCC_CFGR_HPRE_DIV1 0x0    // No division
#define RCC_CFGR_PPRE1_DIV4 0b101 // Division by 4
#define RCC_CFGR_PPRE2_DIV2 0b100 // No division

#define RCC_CFGR_SW_BIT 0x0  // SYSCLK clock selection 0 for HSI, 1 for HSE and 2 for PLL
#define RCC_CFGR_SWS_BIT 0x2 // SYSCLK clock selection status 0 for HSI, 1 for HSE and 2 for PLL

#define FLASH_INTERFACE_REGISTER 0x40023C00
#define FLASH_ACR_LATENCY_BIT 0

void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg);
uint32_t hal_clock_get_sysclk();
uint32_t hal_clock_get_ahbclk();
uint32_t hal_clock_get_apb1clk();
uint32_t hal_clock_get_apb2clk();

#endif // !CORTEX_M4_CLOCK_H