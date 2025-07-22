#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/clock.h"

void uart2_init(uint32_t baudrate)
{

    hal_gpio_set_alternate_function(GPIO_PA02, GPIO_AF07);
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;                                // Enable USART2 clock
    USART2_BRR = (hal_clock_get_apb1clk() + (baudrate / 2)) / baudrate; // BRR clc
    USART2_CR1 = USART_CR1_TE;                                          // Enable transmitter
    USART2_CR1 |= USART_CR1_UE;                                         // Enable USART
}

void uart2_write_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE))
        ;          // Wait until TXE = 1
    USART2_DR = c; // Write data
}

void uart2_write_int(int32_t num)
{
    char buf[12]; // Enough for 32-bit int
    int i = 0;

    if (num == 0)
    {
        uart2_write_char('0');
        return;
    }

    if (num < 0)
    {
        uart2_write_char('-');
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
    {
        uart2_write_char(buf[i]);
    }
}

void uart2_write_float(float num)
{
    if (num < 0)
    {
        uart2_write_char('-');
        num -= num;
    }
    int32_t integer = (int32_t)num;
    uart2_write_int(integer);
    uart2_write_char('.');
    float floating = num - integer;

    integer = (int32_t)(floating*1000000);
    uart2_write_int(integer);
 
}

void uart2_write_string(const char *s)
{
    while (*s)
    {
        uart2_write_char(*s++);
    }
}
