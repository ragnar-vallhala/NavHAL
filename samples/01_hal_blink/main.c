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
    systick_init(1000);
    hal_gpio_setmode(GPIO_PB07, GPIO_OUTPUT, GPIO_PUPD_NONE);
    hal_gpio_setmode(GPIO_PB14, GPIO_OUTPUT, GPIO_PUPD_NONE);
    
    while (1)
    {
        
        hal_gpio_digitalwrite(GPIO_PB07, GPIO_HIGH);
        delay_ms(100);
        hal_gpio_digitalwrite(GPIO_PB07, GPIO_LOW);
        delay_ms(100);
        hal_gpio_digitalwrite(GPIO_PB14, GPIO_HIGH);
        delay_ms(100);
        hal_gpio_digitalwrite(GPIO_PB14, GPIO_LOW);
        delay_ms(100);
    }
}
