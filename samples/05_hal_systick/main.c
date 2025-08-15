/**
 * @file main.c
 * @brief Example: Multiple timers with callbacks and UART output.
 *
 * This example demonstrates:
 * - Configuring the PLL for system clock.
 * - Initializing SysTick timer with 1 ms tick.
 * - Initializing UART2 at 9600 baud for console output.
 * - Setting up TIM2, TIM3, TIM4, TIM5, and TIM9 timers with interrupts.
 * - Attaching callbacks to each timer to print messages via UART2.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */
#define CORTEX_M4
#include "navhal.h"

/**
 * @brief Simple blocking delay (for demonstration purposes)
 *
 * @note This is just a placeholder delay loop.
 */
void delay(void)
{
    for (volatile int i = 0; i < 100000; i++);
}

/** @brief PLL configuration: 8 MHz HSE -> 168 MHz system clock */
hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSE, /**< External crystal */
    .pll_m     = 8,                    /**< PLLM divider */
    .pll_n     = 168,                  /**< PLLN multiplier */
    .pll_p     = 2,                    /**< PLLP division factor */
    .pll_q     = 7                     /**< PLLQ division factor */
};

/** @brief System clock configuration */
hal_clock_config_t cfg = {
    .source = HAL_CLOCK_SOURCE_PLL /**< Use PLL as system clock */
};

/** @brief Timer callback printing message via UART2 */
void print2(void)  { uart2_write("Hello World 2\n\r"); }
void print3(void)  { uart2_write("Hello World 3\n\r"); }
void print4(void)  { uart2_write("Hello World 4\n\r"); }
void print5(void)  { uart2_write("Hello World 5\n\r"); }
void print6(void)  { uart2_write("Hello World 6\n\r"); }
void print7(void)  { uart2_write("Hello World 7\n\r"); }
void print9(void)  { uart2_write("Hello World 9\n\r"); }
void print12(void) { uart2_write("Hello World 12\n\r"); }

/**
 * @brief Main program
 *
 * - Initializes system clock using PLL.
 * - Configures SysTick timer.
 * - Initializes UART2 for console output.
 * - Configures multiple timers with interrupts and attaches callbacks.
 * - Enters infinite loop with optional debug prints.
 */
int main(void)
{
    hal_clock_init(&cfg, &pll_cfg); /**< Initialize system clock with PLL */
    systick_init(1000);             /**< Initialize SysTick with 1 ms tick */
    uart2_init(9600);               /**< Initialize UART2 at 9600 baud */

    /** @brief Initialize TIM2, enable interrupt, attach callback */
    timer_init(TIM2, 0, 21000000);
    timer_enable_interrupt(TIM2);
    timer_attach_callback(TIM2, print2);

    /** @brief Initialize TIM3, enable interrupt, attach callback */
    timer_init(TIM3, 0, 21000000);
    timer_enable_interrupt(TIM3);
    timer_attach_callback(TIM3, print3);

    /** @brief Initialize TIM4, enable interrupt, attach callback */
    timer_init(TIM4, 0, 21000000);
    timer_enable_interrupt(TIM4);
    timer_attach_callback(TIM4, print4);

    /** @brief Initialize TIM5, enable interrupt, attach callback */
    timer_init(TIM5, 0, 21000000);
    timer_enable_interrupt(TIM5);
    timer_attach_callback(TIM5, print5);

    /** @brief Initialize TIM9, enable interrupt, attach callback */
    timer_init(TIM9, 0, 21000000);
    timer_enable_interrupt(TIM9);
    timer_attach_callback(TIM9, print9);

    while (1)
    {
        /** Optional debug prints
         * uart2_write("SysTicks: ");
         * uart2_write(hal_get_tick());
         * uart2_write(", Millis: ");
         * uart2_write(timer_get_count(TIM5));
         * uart2_write("\n\r");
         */
        delay_ms(1000); /**< Optional 1-second delay */
    }
}
