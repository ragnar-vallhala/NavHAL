#define CORTEX_M4
#include "navhal.h"

void uart2_irq_handler(void)
{
    uart2_write_char(uart2_read_char());
}

int main()
{
    systick_init(1000); /**< Initialize SysTick for delays */
    uart2_init(9600);

    hal_interrupt_attach_callback(USART2_IRQn, uart2_irq_handler);
    hal_enable_interrupt(USART2_IRQn);
    while (1)
        ;
}