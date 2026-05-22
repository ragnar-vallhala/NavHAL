/**
 * @file main.c
 * @brief Example application: Toggle LED on GPIO_PA05 based on button input on GPIO_PC13.
 *
 * @details
 * - Initializes SysTick timer with 1 ms tick.
 * - Configures GPIO_PA05 as output (LED).
 * - Configures GPIO_PC13 as input with pull-up (button).
 * - Turns on the LED when the button is pressed, off when released.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

int main(void)
{
    hal_timebase_init(1000);
    hal_gpio_set_mode(GPIO_PA05, HAL_GPIO_MODE_OUTPUT, 0);
    hal_gpio_set_mode(GPIO_PC13, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);

    while (1)
    {
        if (hal_gpio_read(GPIO_PC13))
            hal_gpio_write(GPIO_PA05, HAL_GPIO_LOW);
        else
            hal_gpio_write(GPIO_PA05, HAL_GPIO_HIGH);
    }
}
