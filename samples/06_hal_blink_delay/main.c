/**
 * @file main.c
 * @brief Blink LED on PA05 and print system millis over UART2.
 *
 * This example demonstrates:
 * - Initializing SysTick timer with 40 µs tick.
 * - Configuring GPIO PA05 as output for LED control.
 * - Initializing UART2 at 9600 baud for console output.
 * - Initializing TIM5 timer (prescaler 0, auto-reload 1e9).
 * - Toggling LED and printing system millis in an infinite loop.
 *
 * © 2025 NAVROBOTEC PVT. LTD. All rights reserved.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void) {
    systick_init(40);                                  /**< Initialize SysTick with 40 µs tick */
    uart2_init(9600);                                  /**< Initialize UART2 at 9600 baud */
    hal_gpio_setmode(GPIO_PA05, GPIO_OUTPUT, GPIO_PUPD_NONE); /**< Configure PA05 as output */
    timer_init(TIM5, 0, 1000000000);                  /**< Initialize TIM5 with prescaler 0 and auto-reload 1e9 */

    while (1) {
        hal_gpio_digitalwrite(GPIO_PA05, GPIO_HIGH);   /**< Set PA05 HIGH */
        delay_ms(500);                                 /**< Delay 500 ms */
        hal_gpio_digitalwrite(GPIO_PA05, GPIO_LOW);    /**< Set PA05 LOW */
        delay_ms(500);                                 /**< Delay 500 ms */

        uart2_write("Millis: ");                       /**< Print label */
        uart2_write(hal_get_millis());                 /**< Print system millis */
        uart2_write("\n\r");                           /**< Newline and carriage return */
    }
}
