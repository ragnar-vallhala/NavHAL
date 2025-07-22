#include <stdint.h>
// #include <stdio.h>
#define CORTEX_M4
#include "navhal.h"



// Delay function
void delay(volatile uint32_t count)
{
    while (count--)
        __asm__("nop");
}

int main(void)
{

    uart2_init(9600);
    hal_gpio_setmode(GPIO_PA06, GPIO_INPUT, GPIO_PULLUP);
    hal_gpio_setmode(GPIO_PA05, GPIO_OUTPUT, 0);
    float a = 2.51478963;
    int b = (float)a;
    while (1)
    {
        // printf("Hello");
        uart2_write_string("SYSCLK: ");
        uart2_write_int(hal_clock_get_sysclk());
        uart2_write_string(", AHBCLK: ");
        uart2_write_int(hal_clock_get_ahbclk());
        uart2_write_string(", ABP1CLK: ");
        uart2_write_int(hal_clock_get_apb1clk());
        uart2_write_string(", ABP2CLK: ");
        uart2_write_float(a);
        uart2_write_string("\n");
        delay(100000);
    }
}
