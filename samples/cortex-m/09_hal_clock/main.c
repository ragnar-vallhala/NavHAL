/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file main.c
 * @brief Print system clock and bus clocks over HAL_UART_2.
 *
 * This example demonstrates:
 * - Initializing the PLL for system clock configuration.
 * - Initializing SysTick timer.
 * - Initializing HAL_UART_2 at 9600 baud for console output.
 * - Printing SYSCLK, AHBCLK, APB1CLK, and APB2CLK periodically.
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
    hal_timebase_init(40);               /**< Initialize SysTick with 40 µs tick */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});               /**< Initialize HAL_UART_2 at 9600 baud */

    while (1) {
        hal_uart_print(HAL_UART_2, "sysclk=");           /**< Print SYSCLK label */
        hal_uart_print(HAL_UART_2, hal_clock_get_sysclk()); /**< Print system clock */
        hal_uart_print(HAL_UART_2, ", apb1clk=");        /**< Print APB1CLK label */
        hal_uart_print(HAL_UART_2, hal_clock_get_apb1clk()); /**< Print APB1 clock */
        hal_uart_print(HAL_UART_2, ", apb2clk=");        /**< Print APB2CLK label */
        hal_uart_print(HAL_UART_2, hal_clock_get_apb2clk()); /**< Print APB2 clock */
        hal_uart_print(HAL_UART_2, ", ahbclk=");         /**< Print AHBCLK label */
        hal_uart_print(HAL_UART_2, hal_clock_get_ahbclk()); /**< Print AHB clock */
        hal_uart_print(HAL_UART_2, "\n");                /**< Newline */

        hal_delay_ms(1000);                   /**< Wait 1 second */
    }
}
