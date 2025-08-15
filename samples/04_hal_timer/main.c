/**
 * @file main.c
 * @brief Example application: Print Timer5 count over UART2 continuously.
 *
 * @details
 * - Initializes SysTick with 1 ms tick.
 * - Initializes UART2 at 9600 baud.
 * - Initializes TIM5 timer.
 * - Continuously prints the TIM5 count via UART2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"
#include <stdio.h>  // For sprintf

int main(void) {
    char buffer[20];                    /**< Buffer to hold timer count as string */
    
    systick_init(1000);                 /**< Initialize SysTick with 1 ms tick */
    uart2_init(9600);                   /**< Initialize UART2 at 9600 baud */
    timer_init(TIM5, 0, 1000000000);   /**< Initialize TIM5 with auto-reload 1e9 */

    while (1) {
        sprintf(buffer, "%lu", timer_get_count(TIM5));  /**< Convert timer value to string */
        uart2_write("Timer: ");                          /**< Print label */
        uart2_write(buffer);                             /**< Print TIM5 counter value */
        uart2_write("\n\r");                             /**< Newline and carriage return */
        // delay_ms(10);                                 /**< Optional delay */
    }
}
