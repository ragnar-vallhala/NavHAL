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
    uart6_init(9600);     /**< Initialize UART2 at 9600 baud */
    int n = hal_get_tick();
    int iter = 100;
    while (iter--)
    {
        uart6_write("Hello World\n\r");  /**< Send string over UART2 */
    }
    uart6_write("UART TX NO DMA Test finished: ");
    uart6_write(hal_get_tick() - n);
    uart6_write(" ticks\n\r");
    return 0;
}
