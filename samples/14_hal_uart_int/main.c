#define CORTEX_M4
#include "navhal.h"

void uart2_irq_handler(void)
{
    hal_uart_write_char(HAL_UART_2, hal_uart_read_char(HAL_UART_2));
}

int main()
{
    hal_timebase_init(1000); /**< Initialize SysTick for delays */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});

    hal_interrupt_attach_callback(USART2_IRQn, uart2_irq_handler);
    hal_interrupt_enable(USART2_IRQn);
    while (1)
        ;
}