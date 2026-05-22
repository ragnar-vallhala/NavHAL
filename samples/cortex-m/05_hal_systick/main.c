/**
 * @file main.c
 * @brief Example: Multiple timers with callbacks and UART output.
 *
 * This example demonstrates:
 * - Configuring the system clock using HSI or PLL.
 * - Initializing SysTick timer with 1 ms tick.
 * - Initializing HAL_UART_2 at 9600 baud for console output.
 * - Setting up TIM2, TIM3, TIM4, TIM5, and TIM9 timers with interrupts.
 * - Attaching callbacks to each timer to print messages via HAL_UART_2.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

// System clock configuration
hal_pll_config_t pll_cfg;                       /**< PLL config (unused if HSI) */
hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI}; /**< Use HSI as system clock */

// Timer callback functions
void print2(void)  { hal_uart_print(HAL_UART_2, "Hello World 2\n"); }
void print3(void)  { hal_uart_print(HAL_UART_2, "Hello World 3\n"); }
void print4(void)  { hal_uart_print(HAL_UART_2, "Hello World 4\n"); }
void print5(void)  { hal_uart_print(HAL_UART_2, "Hello World 5\n"); }
void print6(void)  { hal_uart_print(HAL_UART_2, "Hello World 6\n"); }
void print7(void)  { hal_uart_print(HAL_UART_2, "Hello World 7\n"); }
void print9(void)  { hal_uart_print(HAL_UART_2, "Hello World 9\n"); }
void print12(void) { hal_uart_print(HAL_UART_2, "Hello World 12\n"); }

int main(void) {
    // Initialize system clock, SysTick, and UART
    hal_clock_init(&cfg, &pll_cfg);
    hal_timebase_init(1000);  /**< 1 ms tick */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});    /**< HAL_UART_2 at 9600 baud */

    // Initialize timers with interrupts and attach callbacks
    hal_timer_init(TIM2, &(hal_timer_config_t){.prescaler=500, .auto_reload=32000});
    hal_timer_enable_interrupt(TIM2);
    hal_timer_attach_callback(TIM2, print2);

    hal_timer_init(TIM3, &(hal_timer_config_t){.prescaler=500, .auto_reload=32000});
    hal_timer_enable_interrupt(TIM3);
    hal_timer_attach_callback(TIM3, print3);

    hal_timer_init(TIM4, &(hal_timer_config_t){.prescaler=500, .auto_reload=32000});
    hal_timer_enable_interrupt(TIM4);
    hal_timer_attach_callback(TIM4, print4);

    hal_timer_init(TIM5, &(hal_timer_config_t){.prescaler=500, .auto_reload=32000});
    hal_timer_enable_interrupt(TIM5);
    hal_timer_attach_callback(TIM5, print5);

    hal_timer_init(TIM9, &(hal_timer_config_t){.prescaler=500, .auto_reload=32000});
    hal_timer_enable_interrupt(TIM9);
    hal_timer_attach_callback(TIM9, print9);

    // Infinite loop: print system info every second
    while (1) {
        hal_uart_print(HAL_UART_2, "SysTicks: ");
        hal_uart_print(HAL_UART_2, hal_timebase_get_tick());
        hal_uart_print(HAL_UART_2, ", AHB: ");
        hal_uart_print(HAL_UART_2, hal_clock_get_ahbclk());
        hal_uart_print(HAL_UART_2, ", APB1: ");
        hal_uart_print(HAL_UART_2, hal_clock_get_apb1clk());
        hal_uart_print(HAL_UART_2, ", APB2: ");
        hal_uart_print(HAL_UART_2, hal_clock_get_apb2clk());
        hal_uart_print(HAL_UART_2, "\n");

        hal_delay_ms(1000); /**< 1-second delay */
    }
}
