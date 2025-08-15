/**
 * @file main.c
 * @brief Print system clock and bus clocks over UART2.
 *
 * This example demonstrates:
 * - Initializing the PLL for system clock configuration.
 * - Initializing SysTick timer.
 * - Initializing UART2 at 9600 baud for console output.
 * - Printing SYSCLK, AHBCLK, APB1CLK, and APB2CLK periodically.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

/** @brief PLL configuration: 8 MHz HSE -> 168 MHz system clock */
hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSE, /**< External 8 MHz crystal */
    .pll_m = 8,                        /**< PLLM divider */
    .pll_n = 168,                      /**< PLLN multiplier */
    .pll_p = 2,                        /**< PLLP division factor */
    .pll_q = 7                         /**< PLLQ division factor */
};

/** @brief System clock source configuration */
hal_clock_config_t cfg = {
    .source = HAL_CLOCK_SOURCE_PLL /**< Use PLL as system clock */
};

int main(void) {
    hal_clock_init(&cfg, &pll_cfg); /**< Initialize system clock with PLL */
    systick_init(40);               /**< Initialize SysTick with 40 µs tick */
    uart2_init(9600);               /**< Initialize UART2 at 9600 baud */

    while (1) {
        uart2_write("sysclk=");           /**< Print SYSCLK label */
        uart2_write(hal_clock_get_sysclk()); /**< Print system clock */
        uart2_write(", apb1clk=");        /**< Print APB1CLK label */
        uart2_write(hal_clock_get_apb1clk()); /**< Print APB1 clock */
        uart2_write(", apb2clk=");        /**< Print APB2CLK label */
        uart2_write(hal_clock_get_apb2clk()); /**< Print APB2 clock */
        uart2_write(", ahbclk=");         /**< Print AHBCLK label */
        uart2_write(hal_clock_get_ahbclk()); /**< Print AHB clock */
        uart2_write("\n");                /**< Newline */

        delay_ms(1000);                   /**< Wait 1 second */
    }
}
