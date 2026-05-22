/**
 * @file main.c
 * @brief Blink LED on PA05 and print system millis over HAL_UART_2.
 *
 * This example demonstrates:
 * - Initializing SysTick timer with 40 µs tick.
 * - Configuring GPIO PA05 as output for LED control.
 * - Initializing HAL_UART_2 at 9600 baud for console output.
 * - Initializing TIM5 timer (prescaler 0, auto-reload 1e9).
 * - Toggling LED and printing system millis in an infinite loop.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void) {
    hal_timebase_init(40);                                  /**< Initialize SysTick with 40 µs tick */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});                                  /**< Initialize HAL_UART_2 at 9600 baud */
    hal_gpio_set_mode(GPIO_PA05, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE); /**< Configure PA05 as output */
    hal_timer_init(TIM5, &(hal_timer_config_t){.prescaler=0, .auto_reload=1000000000});                  /**< Initialize TIM5 with prescaler 0 and auto-reload 1e9 */

    while (1) {
        hal_gpio_write(GPIO_PA05, HAL_GPIO_HIGH);   /**< Set PA05 HIGH */
        hal_delay_ms(500);                                 /**< Delay 500 ms */
        hal_gpio_write(GPIO_PA05, HAL_GPIO_LOW);    /**< Set PA05 LOW */
        hal_delay_ms(500);                                 /**< Delay 500 ms */

        hal_uart_print(HAL_UART_2, "Millis: ");                       /**< Print label */
        hal_uart_print(HAL_UART_2, hal_timebase_get_millis());                 /**< Print system millis */
        hal_uart_print(HAL_UART_2, "\n\r");                           /**< Newline and carriage return */
    }
}
