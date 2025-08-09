/**
 * @file clock.c
 * @brief Cortex-M4 (STM32F4) Clock HAL Implementation.
 *
 * Provides functions to initialize and retrieve clock frequencies
 * including SYSCLK, AHB, APB1, and APB2 clocks.
 *
 * This implementation supports HSI, HSE, and PLL clock sources.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-21
 */

#include "core/cortex-m4/clock.h"

/**
 * @brief Initialize system clocks based on the provided configuration.
 *
 * Enables and waits for the selected clock source (HSI, HSE, or PLL).
 * If PLL is selected, configures PLL parameters and switches system clock
 * source to PLL output.
 *
 * @param cfg Pointer to clock configuration structure specifying source.
 * @param pll_cfg Pointer to PLL configuration structure (used if source is PLL).
 */
void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg)
{
    // Enable and wait for selected clock source
    if (cfg->source == HAL_CLOCK_SOURCE_HSE)
    {
        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) |= (1 << RCC_CR_HSE_ON_BIT);
        while (!(((*RCC_CR) >> RCC_CR_HSE_READY_BIT) & 0x1))
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_HSI)
    {
        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) |= (1 << RCC_CR_HSI_ON_BIT);
        while (!(((*RCC_CR) >> RCC_CR_HSI_READY_BIT) & 0x1))
            ;
    }

    // PLL configuration and enabling (if PLL is selected)
    if (cfg->source == HAL_CLOCK_SOURCE_PLL)
    {
        // Enable PLL input source clock and wait for readiness
        if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSE)
        {
            volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
            (*RCC_CR) |= (1 << RCC_CR_HSE_ON_BIT);
            while (!(((*RCC_CR) >> RCC_CR_HSE_READY_BIT) & 0x1))
                ;
        }
        else if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI)
        {
            volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
            (*RCC_CR) |= (1 << RCC_CR_HSI_ON_BIT);
            while (!(((*RCC_CR) >> RCC_CR_HSI_READY_BIT) & 0x1))
                ;
        }

        // Turn off PLL before configuring
        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) &= ~(1 << RCC_CR_PLL_ON_BIT);
        while ((((*RCC_CR) >> RCC_CR_PLL_READY_BIT) & 0x1))
            ;

        // Configure PLL registers
        volatile uint32_t *const RCC_PLLCFGR = (volatile uint32_t *)(RCC + RCC_PLLCFGR_OFFSET);
        *RCC_PLLCFGR = 0; // Clear existing config

        // Set PLL source (HSI=0, HSE=1)
        if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI)
        {
            (*RCC_PLLCFGR) &= ~(1 << RCC_PLLCFGR_SRC_BIT);
        }
        else
        {
            (*RCC_PLLCFGR) |= (1 << RCC_PLLCFGR_SRC_BIT);
        }

        // Set PLL dividers and multipliers
        (*RCC_PLLCFGR) |= (((pll_cfg->pll_m % 64) << RCC_PLLCFGR_PLLM_BIT)) |
                          (((pll_cfg->pll_n % 433) << RCC_PLLCFGR_PLLN_BIT)) |
                          ((((pll_cfg->pll_p >> 1) - 1) << RCC_PLLCFGR_PLLP_BIT)) |
                          (((pll_cfg->pll_q) << RCC_PLLCFGR_PLLQ_BIT));

        // Turn PLL back on and wait for it to lock
        (*RCC_CR) |= (1 << RCC_CR_PLL_ON_BIT);
        while (!(((*RCC_CR) >> RCC_CR_PLL_READY_BIT) & 0x1))
            ;
    }

    // Configure flash latency (hardcoded 5 WS here, ideally adapt to SYSCLK)
    volatile uint32_t *const FLASH_ACR = (volatile uint32_t *)(FLASH_INTERFACE_REGISTER);
    (*FLASH_ACR) &= ~(0x7 << FLASH_ACR_LATENCY_BIT); // Clear latency bits
    (*FLASH_ACR) |= (5 << FLASH_ACR_LATENCY_BIT);    // Set to 5 wait states

    // Configure AHB, APB1, APB2 prescalers
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    (*RCC_CFGR) &= ~((0xF << RCC_CFGR_HPRE_BIT) | (0x7 << RCC_CFGR_PPRE1_BIT) | (0x7 << RCC_CFGR_PPRE2_BIT));

    (*RCC_CFGR) |= (RCC_CFGR_HPRE_DIV1 << RCC_CFGR_HPRE_BIT);   // AHB prescaler = 1
    (*RCC_CFGR) |= (RCC_CFGR_PPRE1_DIV4 << RCC_CFGR_PPRE1_BIT); // APB1 prescaler = 4
    (*RCC_CFGR) |= (RCC_CFGR_PPRE2_DIV2 << RCC_CFGR_PPRE2_BIT); // APB2 prescaler = 2

    // Switch system clock source
    if (cfg->source == HAL_CLOCK_SOURCE_HSI)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT); // Select HSI
        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 0)
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_HSE)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
        (*RCC_CFGR) |= (1 << RCC_CFGR_SW_BIT); // Select HSE
        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 1)
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_PLL)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
        (*RCC_CFGR) |= (2 << RCC_CFGR_SW_BIT); // Select PLL
        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 2)
            ;
    }
}

/**
 * @brief Get the current system clock frequency in Hz.
 *
 * Detects the current SYSCLK source and calculates frequency based on
 * configured clock source and PLL parameters.
 *
 * @return SYSCLK frequency in Hertz.
 */
uint32_t hal_clock_get_sysclk(void)
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    volatile uint32_t *const RCC_PLLCFGR = (volatile uint32_t *)(RCC + RCC_PLLCFGR_OFFSET);

    uint32_t sysclk;
    uint8_t sws = ((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3;

    switch (sws)
    {
    case 0:                // HSI selected
        sysclk = 16000000; // Internal HSI clock frequency
        break;
    case 1:               // HSE selected
        sysclk = 8000000; // External crystal frequency (should be configurable)
        break;
    case 2: // PLL selected
    {
        uint32_t pllcfgr = *RCC_PLLCFGR;

        uint32_t pll_m = (pllcfgr >> RCC_PLLCFGR_PLLM_BIT) & 0x3F;
        uint32_t pll_n = (pllcfgr >> RCC_PLLCFGR_PLLN_BIT) & 0x1FF;
        uint32_t pll_p = (((pllcfgr >> RCC_PLLCFGR_PLLP_BIT) & 0x3) + 1) * 2;

        uint32_t pll_src = (pllcfgr >> RCC_PLLCFGR_SRC_BIT) & 0x1;
        uint32_t vco_in = pll_src ? 8000000 : 16000000; // 8MHz for HSE, 16MHz for HSI

        sysclk = (vco_in / pll_m) * pll_n / pll_p;
        break;
    }
    default:
        sysclk = 0; // Unknown clock source/error
        break;
    }

    return sysclk;
}

/**
 * @brief Decode AHB prescaler register value to division factor.
 *
 * @param val 4-bit prescaler value from RCC_CFGR register.
 * @return Division factor (1, 2, 4, 8, 16, 64, 128, 256, 512).
 */
static uint32_t _decode_prescaler(uint32_t val)
{
    switch (val)
    {
    case 0x0:
        return 1;
    case 0x8:
        return 2;
    case 0x9:
        return 4;
    case 0xA:
        return 8;
    case 0xB:
        return 16;
    case 0xC:
        return 64;
    case 0xD:
        return 128;
    case 0xE:
        return 256;
    case 0xF:
        return 512;
    default:
        return 1;
    }
}

/**
 * @brief Decode APB prescaler register value to division factor.
 *
 * @param val 3-bit prescaler value from RCC_CFGR register.
 * @return Division factor (1, 2, 4, 8, 16).
 */
static uint32_t _decode_apb_prescaler(uint32_t val)
{
    switch (val)
    {
    case 0x0:
        return 1;
    case 0x4:
        return 2;
    case 0x5:
        return 4;
    case 0x6:
        return 8;
    case 0x7:
        return 16;
    default:
        return 1;
    }
}

/**
 * @brief Get the current AHB clock frequency.
 *
 * Calculated as SYSCLK divided by AHB prescaler.
 *
 * @return AHB bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_ahbclk(void)
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    uint32_t prescaler = ((*RCC_CFGR) >> RCC_CFGR_HPRE_BIT) & 0xF;
    return hal_clock_get_sysclk() / _decode_prescaler(prescaler);
}

/**
 * @brief Get the current APB1 clock frequency.
 *
 * Calculated as SYSCLK divided by APB1 prescaler.
 *
 * @return APB1 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb1clk(void)
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    uint32_t prescaler = ((*RCC_CFGR) >> RCC_CFGR_PPRE1_BIT) & 0x7;
    return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}

/**
 * @brief Get the current APB2 clock frequency.
 *
 * Calculated as SYSCLK divided by APB2 prescaler.
 *
 * @return APB2 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb2clk(void)
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    uint32_t prescaler = ((*RCC_CFGR) >> RCC_CFGR_PPRE2_BIT) & 0x7;
    return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}
