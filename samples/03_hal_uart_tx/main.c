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
    
    
    while (1)
    {
        // printf("Hello");
        uart2_write_string("Hello World\n");
        
        delay(100000);
    }
}
