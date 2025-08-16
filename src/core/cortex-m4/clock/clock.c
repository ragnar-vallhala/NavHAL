/**
 * @file clock.c
 * @brief Cortex-M4 (STM32F4) Clock HAL Implementation
 *
 * @details
 * This file provides a hardware abstraction layer for clock configuration on STM32F4 microcontrollers.
 * It includes functions to:
 * - Initialize system clock sources (HSI, HSE, PLL)
 * - Configure PLL parameters (M, N, P, Q factors)
 * - Set AHB/APB bus prescalers
 * - Manage flash wait states
 * - Retrieve current clock frequencies for all system buses
 *
 * @note The implementation assumes an STM32F4xx microcontroller and uses direct register access
 * for maximum performance and minimal overhead.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/flash_reg.h"
#include "core/cortex-m4/rcc_reg.h"
#include <stdint.h>

/**
 * @brief Internal function to enable/disable HSE clock
 * @param state RCC_ON (1) to enable, RCC_OFF (0) to disable
 * @note Blocks until clock ready flag matches requested state
 */
static void _toggle_hse_clock(uint8_t state)
{
    if (state) RCC->CR |= RCC_CR_HSEON;
    else RCC->CR &= ~RCC_CR_HSEON;
    while (((RCC->CR & RCC_CR_HSERDY) != 0) != (state));
}

/**
 * @brief Internal function to enable/disable HSI clock
 * @param state RCC_ON (1) to enable, RCC_OFF (0) to disable
 * @note Blocks until clock ready flag matches requested state
 */
static void _toggle_hsi_clock(uint8_t state)
{
    if (state) RCC->CR |= RCC_CR_HSION;
    else RCC->CR &= ~RCC_CR_HSION;
    while (((RCC->CR & RCC_CR_HSIRDY) != 0) != (state));
}

/**
 * @brief Internal function to enable/disable PLL clock
 * @param state RCC_ON (1) to enable, RCC_OFF (0) to disable
 * @note Blocks until clock ready flag matches requested state
 */
static void _toggle_pll_clock(uint8_t state)
{
    if (state) RCC->CR |= RCC_CR_PLLON;
    else RCC->CR &= ~RCC_CR_PLLON;
    while (((RCC->CR & RCC_CR_PLLRDY) != 0) != (state));
}

/**
 * @brief Initialize system clock and bus prescalers
 * @param[in] cfg Pointer to clock configuration structure
 * @param[in] pll_cfg Pointer to PLL configuration structure (required if using PLL)
 *
 * @details
 * Configures the complete clock tree including:
 * - Clock source selection (HSI/HSE/PLL)
 * - PLL configuration (when used)
 * - Flash latency settings
 * - AHB/APB prescalers
 *
 * @note This function will block until clock switches are complete
 */
void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg)
{
    /* Enable selected clock source */
    if (cfg->source == HAL_CLOCK_SOURCE_HSE) _toggle_hse_clock(RCC_ON);
    else if (cfg->source == HAL_CLOCK_SOURCE_HSI) _toggle_hsi_clock(RCC_ON);
    else if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
        if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSE) _toggle_hse_clock(RCC_ON);
        else if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI) _toggle_hsi_clock(RCC_ON);

        /* Configure PLL parameters */
        _toggle_pll_clock(RCC_OFF);
        RCC->PLLCFGR = 0;

        if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI) RCC->PLLCFGR &= ~RCC_PLLCFGR_SRC;
        else RCC->PLLCFGR |= RCC_PLLCFGR_SRC;

        RCC->PLLCFGR |= RCC_PLLCFGR_PLLM(pll_cfg->pll_m) |
                        RCC_PLLCFGR_PLLN(pll_cfg->pll_n) |
                        RCC_PLLCFGR_PLLP(pll_cfg->pll_p) |
                        RCC_PLLCFGR_PLLQ(pll_cfg->pll_q);

        _toggle_pll_clock(RCC_ON);
    }

    /* Configure flash latency (fixed at 5 wait states) */
    volatile uint32_t *const FLASH_ACR = (volatile uint32_t *)FLASH_INTERFACE_REGISTER;
    (*FLASH_ACR) &= ~(0x7 << FLASH_ACR_LATENCY_BIT);
    (*FLASH_ACR) |= (5 << FLASH_ACR_LATENCY_BIT);

    /* Set bus prescalers */
    RCC->CFGR &= ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK | RCC_CFGR_PPRE2_MASK);
    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 << RCC_CFGR_HPRE_BIT);
    RCC->CFGR |= (RCC_CFGR_PPRE1_DIV4 << RCC_CFGR_PPRE1_BIT);
    RCC->CFGR |= (RCC_CFGR_PPRE2_DIV2 << RCC_CFGR_PPRE2_BIT);

    /* Switch system clock source */
    if (cfg->source == HAL_CLOCK_SOURCE_HSI) {
        RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
        while (((RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3) != 0);
    } else if (cfg->source == HAL_CLOCK_SOURCE_HSE) {
        RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
        RCC->CFGR |= (1 << RCC_CFGR_SW_BIT);
        while (((RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3) != 1);
    } else if (cfg->source == HAL_CLOCK_SOURCE_PLL) {
        RCC->CFGR &= ~(0x3 << RCC_CFGR_SW_BIT);
        RCC->CFGR |= (2 << RCC_CFGR_SW_BIT);
        while (((RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3) != 2);
    }
}

/**
 * @brief Get current system clock (SYSCLK) frequency
 * @return SYSCLK frequency in Hz
 *
 * @details
 * Determines SYSCLK frequency based on current clock source:
 * - HSI: Fixed 16 MHz
 * - HSE: Fixed 8 MHz
 * - PLL: Calculated from PLL configuration registers
 */
uint32_t hal_clock_get_sysclk(void)
{
    uint32_t sysclk;
    uint8_t sws = (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3;
    
    switch (sws) {
        case 0: sysclk = 16000000; break; // HSI
        case 1: sysclk = 8000000; break;  // HSE
        case 2: {
            uint32_t pll_m = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLM_BIT) & 0x3F;
            uint32_t pll_n = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLN_BIT) & 0x1FF;
            uint32_t pll_p = (((RCC->PLLCFGR >> RCC_PLLCFGR_PLLP_BIT) & 0x3) + 1) * 2;
            uint32_t pll_src = (RCC->PLLCFGR >> RCC_PLLCFGR_SRC_BIT) & 0x1;
            uint32_t vco_in = pll_src ? 8000000 : 16000000;
            sysclk = (vco_in / pll_m) * pll_n / pll_p;
            break;
        }
        default: sysclk = 0; break;
    }
    return sysclk;
}

/**
 * @brief Decode AHB prescaler value from register
 * @param val Raw prescaler register value
 * @return Division factor (1, 2, 4, 8, 16, 64, 128, 256, or 512)
 */
static uint32_t _decode_prescaler(uint32_t val)
{
    switch(val) {
        case 0x0: return 1;
        case 0x8: return 2;
        case 0x9: return 4;
        case 0xA: return 8;
        case 0xB: return 16;
        case 0xC: return 64;
        case 0xD: return 128;
        case 0xE: return 256;
        case 0xF: return 512;
        default: return 1;
    }
}

/**
 * @brief Decode APB prescaler value from register
 * @param val Raw prescaler register value
 * @return Division factor (1, 2, 4, 8, or 16)
 */
static uint32_t _decode_apb_prescaler(uint32_t val)
{
    switch(val) {
        case 0x0: return 1;
        case 0x4: return 2;
        case 0x5: return 4;
        case 0x6: return 8;
        case 0x7: return 16;
        default: return 1;
    }
}

/**
 * @brief Get current AHB bus frequency
 * @return AHB clock frequency in Hz
 */
uint32_t hal_clock_get_ahbclk(void)
{
    uint32_t prescaler = (RCC->CFGR >> RCC_CFGR_HPRE_BIT) & 0xF;
    return hal_clock_get_sysclk() / _decode_prescaler(prescaler);
}

/**
 * @brief Get current APB1 bus frequency
 * @return APB1 clock frequency in Hz
 */
uint32_t hal_clock_get_apb1clk(void)
{
    uint32_t prescaler = (RCC->CFGR >> RCC_CFGR_PPRE1_BIT) & 0x7;
    return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}

/**
 * @brief Get current APB2 bus frequency
 * @return APB2 clock frequency in Hz
 */
uint32_t hal_clock_get_apb2clk(void)
{
    uint32_t prescaler = (RCC->CFGR >> RCC_CFGR_PPRE2_BIT) & 0x7;
    return hal_clock_get_sysclk() / _decode_apb_prescaler(prescaler);
}