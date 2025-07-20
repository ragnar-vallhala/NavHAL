#define CORTEX_M4
#include "navhal.h"

void delay(void)
{
    for (volatile int i = 0; i < 50000; i++)
        ;
}

int main(void)
{
    hal_gpio_setmode(GPIO_PA05, GPIO_OUTPUT, 0);
    if (hal_gpio_getmode(GPIO_PA05) != GPIO_OUTPUT)
        while (1)
            ;
    while (1)
    {
        hal_gpio_digitalwrite(GPIO_PA05, GPIO_HIGH);
        delay();
        hal_gpio_digitalwrite(GPIO_PA05, GPIO_LOW);
        delay();
    }
}
