/**
 * @file main.c
 * @brief Example: Multiple timers with callbacks and UART output.
 *
 * This example demonstrates:
 * - Configuring the system clock using HSI or PLL.
 * - Initializing SysTick timer with 1 ms tick.
 * - Initializing UART2 at 9600 baud for console output.
 * - Setting up TIM2, TIM3, TIM4, TIM5, and TIM9 timers with interrupts.
 * - Attaching callbacks to each timer to print messages via UART2.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

// System clock configuration
hal_pll_config_t pll_cfg;                       /**< PLL config (unused if HSI) */
hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI}; /**< Use HSI as system clock */

// Timer callback functions
void print2(void)  { uart2_write("Hello World 2\n"); }
void print3(void)  { uart2_write("Hello World 3\n"); }
void print4(void)  { uart2_write("Hello World 4\n"); }
void print5(void)  { uart2_write("Hello World 5\n"); }
void print6(void)  { uart2_write("Hello World 6\n"); }
void print7(void)  { uart2_write("Hello World 7\n"); }
void print9(void)  { uart2_write("Hello World 9\n"); }
void print12(void) { uart2_write("Hello World 12\n"); }

int main(void) {
    // Initialize system clock, SysTick, and UART
    hal_clock_init(&cfg, &pll_cfg);
    systick_init(1000);  /**< 1 ms tick */
    uart2_init(9600);    /**< UART2 at 9600 baud */

    // Initialize timers with interrupts and attach callbacks
    timer_init(TIM2, 500, 32000);
    timer_enable_interrupt(TIM2);
    timer_attach_callback(TIM2, print2);

    timer_init(TIM3, 500, 32000);
    timer_enable_interrupt(TIM3);
    timer_attach_callback(TIM3, print3);

    timer_init(TIM4, 500, 32000);
    timer_enable_interrupt(TIM4);
    timer_attach_callback(TIM4, print4);

    timer_init(TIM5, 500, 32000);
    timer_enable_interrupt(TIM5);
    timer_attach_callback(TIM5, print5);

    timer_init(TIM9, 500, 32000);
    timer_enable_interrupt(TIM9);
    timer_attach_callback(TIM9, print9);

    // Infinite loop: print system info every second
    while (1) {
        uart2_write("SysTicks: ");
        uart2_write(hal_get_tick());
        uart2_write(", AHB: ");
        uart2_write(hal_clock_get_ahbclk());
        uart2_write(", APB1: ");
        uart2_write(hal_clock_get_apb1clk());
        uart2_write(", APB2: ");
        uart2_write(hal_clock_get_apb2clk());
        uart2_write("\n");

        delay_ms(1000); /**< 1-second delay */
    }
}
