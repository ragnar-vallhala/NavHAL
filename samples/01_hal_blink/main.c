#include "core/cortex-m4/gpio.h"

void delay(void)
{
    for (volatile int i = 0; i < 100000; i++)
        ;
}


int main(void)
{
    hal_gpio_setmode(GPIO_PA05,OUTPUT);
    while (1)
    {
        hal_gpio_digitalwrite(GPIO_PA05,HIGH);
        delay();
        hal_gpio_digitalwrite(GPIO_PA05,LOW);
        delay();
    }
}
