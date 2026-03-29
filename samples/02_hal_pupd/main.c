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
    systick_init(1000);
    hal_gpio_setmode(GPIO_PB07, GPIO_OUTPUT, 0);
    hal_gpio_setmode(GPIO_PC13, GPIO_INPUT, GPIO_PULLUP);

    while (1)
    {
        if (hal_gpio_digitalread(GPIO_PC13)==0)
            hal_gpio_digitalwrite(GPIO_PB07, GPIO_LOW);
        else
            hal_gpio_digitalwrite(GPIO_PB07, GPIO_HIGH);
    }
}
