/**
 * @file main.c
 * @brief Example application: Send "Hello World" via HAL_UART_2 every 1 second.
 *
 * @details
 * - Initializes SysTick timer with 1 ms tick.
 * - Initializes HAL_UART_2 peripheral with 9600 baud rate.
 * - Sends "Hello World" over HAL_UART_2 in a 1-second loop.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    hal_timebase_init(1000);   /**< Initialize SysTick with 1 ms tick */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});     /**< Initialize HAL_UART_2 at 9600 baud */
    int n = hal_timebase_get_tick();
    int iter = 100;
    while (iter--)
    {
        hal_uart_print(HAL_UART_2, "Hello World\n\r");  /**< Send string over HAL_UART_2 */
    }
    hal_uart_print(HAL_UART_2, "UART TX NO DMA Test finished: ");
    hal_uart_print(HAL_UART_2, hal_timebase_get_tick() - n);
    hal_uart_print(HAL_UART_2, " ticks\n\r");
    return 0;
}
