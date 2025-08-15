#define CORTEX_M4
#include "navhal.h"



// Delay function


int main(void)
{
    systick_init(1000);
    uart2_init(9600);
    
    
    while (1)
    {
        // printf("Hello");
        uart2_write_string("Hello World\n");
        
        delay_ms(1000);
    }
}
