/**
 * @file main.c
 * @brief Example application toggling an LED on GPIO_PA05 using HAL.
 *
 * @details
 * - Initializes SysTick timer with 1 ms tick.
 * - Configures GPIO_PA05 as output with no pull-up/pull-down.
 * - Toggles the LED with 100 ms delay in an infinite loop.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    hal_timebase_init(1000);
    hal_gpio_set_mode(GPIO_PA05, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
    
    while (1)
    {
        hal_gpio_write(GPIO_PA05, HAL_GPIO_HIGH);
        hal_delay_ms(100);
        hal_gpio_write(GPIO_PA05, HAL_GPIO_LOW);
        hal_delay_ms(100);
    }
}
