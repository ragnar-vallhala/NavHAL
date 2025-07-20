#include "core/cortex-m4/gpio.h"

void delay(void)
{
    for (volatile int i = 0; i < 500000; i++)
        ;
}

int main(void)
{
    hal_gpio_setmode(GPIO_PA05, OUTPUT, 0);
    hal_gpio_setmode(GPIO_PA06, INPUT, GPIO_PULLDOWN);
    if (hal_gpio_getmode(GPIO_PA05) != OUTPUT)
        while (1)
            ;
    hal_gpio_digitalwrite(GPIO_PA05, HIGH); // Test LED
    delay();

    while (1)
    {
        if (hal_gpio_digitalread(GPIO_PA06))
            hal_gpio_digitalwrite(GPIO_PA05, HIGH);
        else
            hal_gpio_digitalwrite(GPIO_PA05, LOW);
    }
}
