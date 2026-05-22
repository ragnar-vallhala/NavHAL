/**
 * @file main.c
 * @brief Example application demonstrating DWT cycle counter for timing.
 *
 * @details
 * - Initializes the clock and HAL_UART_2 for output.
 * - Initializes DWT cycle counter.
 * - Measures and displays the number of processor cycles consumed by a float
 * workload.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"
#include <stdint.h>

/** @brief PLL configuration: 16 MHz HSI -> 84 MHz system clock */
hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSI, /**< Internal 16 MHz oscillator */
    .pll_m = 16,                       /**< PLLM divider (16MHz / 16 = 1MHz) */
    .pll_n = 336, /**< PLLN multiplier (1MHz * 336 = 336MHz) */
    .pll_p = 4,   /**< PLLP division factor (336MHz / 4 = 84MHz) */
    .pll_q = 7    /**< PLLQ division factor */
};

/** @brief System clock source configuration */
hal_clock_config_t clock_cfg = {
    .source = HAL_CLOCK_SOURCE_PLL, /**< Use PLL as system clock */
    .hpre_div = RCC_CFGR_HPRE_DIV1,
    .ppre1_div = RCC_CFGR_PPRE_DIV2,
    .ppre2_div = RCC_CFGR_PPRE_DIV1};

int main(void) {
  // Initialize system
  hal_fpu_enable();
  hal_clock_init(&clock_cfg, &pll_cfg);
  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});

  // Initialize DWT
  hal_cycle_counter_init();

  hal_uart_print(HAL_UART_2, "\r\nNavHAL DWT Logic Timing Sample\r\n");
  hal_uart_print(HAL_UART_2, "Measures cycles taken by a computational workload.\r\n");

  while (1) {
    uint32_t start = hal_cycle_counter_get();

    // Computational workload: perform some float math in a loop
    volatile float f = 1.2345f;
    for (int i = 0; i < 1000; i++) {
      f = f * 1.001f + 0.001f;
    }

    uint32_t end = hal_cycle_counter_get();
    uint32_t elapsed = end - start;

    // Report results
    hal_uart_print(HAL_UART_2, "Workload iterations: 1000\r\n");
    hal_uart_print(HAL_UART_2, "Processor cycles: ");
    hal_uart_print(HAL_UART_2, elapsed);
    hal_uart_print(HAL_UART_2, "\r\n");

    // Reset cycles for next run
    hal_cycle_counter_reset();

    hal_delay_ms(2000);
  }
}
