/**
 * @file main.c
 * @brief Example application: Send "Hello World" via UART2 every 1 second.
 *
 * @details
 * - Initializes SysTick timer with 1 ms tick.
 * - Initializes UART2 peripheral with 9600 baud rate.
 * - Sends "Hello World" over UART2 in a 1-second loop.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    systick_init(1000);   /**< Initialize SysTick with 1 ms tick */
    uart2_init(9600);     /**< Initialize UART2 at 9600 baud */

    while (1)
    {
        uart2_write_string("Hello World\n");  /**< Send string over UART2 */
        delay_ms(1000);                       /**< Wait for 1 second */
    }
}
