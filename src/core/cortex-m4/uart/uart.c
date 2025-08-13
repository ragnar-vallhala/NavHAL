/**
 * @file uart.c
 * @brief UART initialization and I/O functions for STM32F4 UART1, UART2, and UART6.
 *
 * @details
 * This file provides basic UART setup and read/write functionality for three UART peripherals.
 * It supports initialization with baudrate, character, integer, float, and string transmission,
 * as well as reading characters and checking availability.
 */

#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/clock.h"

/**
 * @brief Initialize UART peripheral with specified baudrate.
 *
 * @param baudrate Desired UART baudrate.
 * @param uart UART peripheral identifier (UART1, UART2, UART6).
 */
void uart_init(uint32_t baudrate, hal_uart_t uart)
{
    if (uart == UART2)
        uart2_init(baudrate);
    else if (uart == UART1)
        uart1_init(baudrate);
    else if (uart == UART6)
        uart6_init(baudrate);
}

/**
 * @brief Write a single character to UART.
 *
 * @param c Character to send.
 * @param uart UART peripheral identifier.
 */
void uart_write_char(char c, hal_uart_t uart)
{
    if (uart == UART2)
        uart2_write_char(c);
    else if (uart == UART1)
        uart1_write_char(c);
    else if (uart == UART6)
        uart6_write_char(c);
}

/**
 * @brief Write an integer as ASCII characters to UART.
 *
 * @param num Integer to send.
 * @param uart UART peripheral identifier.
 */
void uart_write_int(int32_t num, hal_uart_t uart)
{
    if (uart == UART2)
        uart2_write_int(num);
    else if (uart == UART1)
        uart1_write_int(num);
    else if (uart == UART6)
        uart6_write_int(num);
}

/**
 * @brief Write a floating-point number to UART as ASCII.
 *
 * @param num Float value to send.
 * @param uart UART peripheral identifier.
 */
void uart_write_float(float num, hal_uart_t uart)
{
    if (uart == UART2)
        uart2_write_float(num);
    else if (uart == UART1)
        uart1_write_float(num);
    else if (uart == UART6)
        uart6_write_float(num);
}

/**
 * @brief Write a null-terminated string to UART.
 *
 * @param s Pointer to string to send.
 * @param uart UART peripheral identifier.
 */
void uart_write_string(const char *s, hal_uart_t uart)
{
    if (uart == UART2)
        uart2_write_string(s);
    else if (uart == UART1)
        uart1_write_string(s);
    else if (uart == UART6)
        uart6_write_string(s);
}

/**
 * @brief Initialize UART1 peripheral.
 *
 * @param baudrate Desired baudrate.
 *
 * @note Configures GPIO PA9 for alternate function AF7, enables USART1 clock,
 *       sets baudrate register, enables transmitter, and enables USART.
 */
void uart1_init(uint32_t baudrate)
{
    hal_gpio_set_alternate_function(GPIO_PA09, GPIO_AF07);
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1_BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) / baudrate;
    USART1_CR1 = USART_CR1_TE;
    USART1_CR1 |= USART_CR1_UE;
}

/**
 * @brief Initialize UART6 peripheral.
 *
 * @param baudrate Desired baudrate.
 *
 * @note Configures GPIO PC6 for alternate function AF7, enables USART6 clock,
 *       sets baudrate register, enables transmitter, and enables USART.
 */
void uart6_init(uint32_t baudrate)
{
    hal_gpio_set_alternate_function(GPIO_PC06, GPIO_AF07);
    RCC_APB2ENR |= RCC_APB2ENR_USART6EN;
    USART6_BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) / baudrate;
    USART6_CR1 = USART_CR1_TE;
    USART6_CR1 |= USART_CR1_UE;
}

/**
 * @brief Initialize UART2 peripheral.
 *
 * @param baudrate Desired baudrate.
 *
 * @note Configures GPIO PA2 and PA3 for alternate function AF7, enables USART2 clock,
 *       sets baudrate register, enables transmitter and receiver, and enables USART.
 */
void uart2_init(uint32_t baudrate)
{
    hal_gpio_set_alternate_function(GPIO_PA02, GPIO_AF07);
    hal_gpio_set_alternate_function(GPIO_PA03, GPIO_AF07);
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2_BRR = (hal_clock_get_apb1clk() + (baudrate / 2)) / baudrate;
    USART2_CR1 = USART_CR1_TE | USART_CR1_RE;
    USART2_CR1 |= USART_CR1_UE;
}

/**
 * @brief Write a character to UART2.
 *
 * @param c Character to send.
 *
 * @note Waits until transmit data register is empty.
 */
void uart2_write_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE))
        ;
    USART2_DR = c;
}

/**
 * @brief Write a character to UART1.
 *
 * @param c Character to send.
 *
 * @note Waits until transmit data register is empty.
 */
void uart1_write_char(char c)
{
    while (!(USART1_SR & USART_SR_TXE))
        ;
    USART1_DR = c;
}

/**
 * @brief Write a character to UART6.
 *
 * @param c Character to send.
 *
 * @note Waits until transmit data register is empty.
 */
void uart6_write_char(char c)
{
    while (!(USART6_SR & USART_SR_TXE))
        ;
    USART6_DR = c;
}

/**
 * @brief Write an integer to UART2 as ASCII.
 *
 * @param num Integer to send.
 */
void uart2_write_int(int32_t num)
{
    char buf[12];
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

/**
 * @brief Write an integer to UART1 as ASCII.
 *
 * @param num Integer to send.
 */
void uart1_write_int(int32_t num)
{
    char buf[12];
    int i = 0;

    if (num == 0)
    {
        uart1_write_char('0');
        return;
    }

    if (num < 0)
    {
        uart1_write_char('-');
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
    {
        uart1_write_char(buf[i]);
    }
}

/**
 * @brief Write an integer to UART6 as ASCII.
 *
 * @param num Integer to send.
 */
void uart6_write_int(int32_t num)
{
    char buf[12];
    int i = 0;

    if (num == 0)
    {
        uart6_write_char('0');
        return;
    }

    if (num < 0)
    {
        uart6_write_char('-');
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
    {
        uart6_write_char(buf[i]);
    }
}

/**
 * @brief Write a floating-point number to UART2.
 *
 * @param num Float value to send.
 *
 * @note Prints sign, integer part, decimal point, and 5 decimal digits of fraction.
 */
void uart2_write_float(float num)
{
    if (num < 0)
    {
        uart2_write_char('-');
        num = -num;
    }
    int32_t integer = (int32_t)num;
    uart2_write_int(integer);
    uart2_write_char('.');
    float floating = num - integer;
    uart2_write_int((int32_t)(floating * 100000 + 0.5f));
}

/**
 * @brief Write a floating-point number to UART1.
 *
 * @param num Float value to send.
 */
void uart1_write_float(float num)
{
    if (num < 0)
    {
        uart1_write_char('-');
        num = -num;
    }
    int32_t integer = (int32_t)num;
    uart1_write_int(integer);
    uart1_write_char('.');
    float floating = num - integer;
    uart1_write_int((int32_t)(floating * 100000 + 0.5f));
}

/**
 * @brief Write a floating-point number to UART6.
 *
 * @param num Float value to send.
 */
void uart6_write_float(float num)
{
    if (num < 0)
    {
        uart6_write_char('-');
        num = -num;
    }
    int32_t integer = (int32_t)num;
    uart6_write_int(integer);
    uart6_write_char('.');
    float floating = num - integer;
    uart6_write_int((int32_t)(floating * 100000 + 0.5f));
}

/**
 * @brief Write a null-terminated string to UART2.
 *
 * @param s Pointer to string to send.
 */
void uart2_write_string(const char *s)
{
    while (*s)
    {
        uart2_write_char(*s++);
    }
}

/**
 * @brief Write a null-terminated string to UART1.
 *
 * @param s Pointer to string to send.
 */
void uart1_write_string(const char *s)
{
    while (*s)
    {
        uart1_write_char(*s++);
    }
}

/**
 * @brief Write a null-terminated string to UART6.
 *
 * @param s Pointer to string to send.
 */
void uart6_write_string(const char *s)
{
    while (*s)
    {
        uart6_write_char(*s++);
    }
}

/**
 * @brief Read a single character from UART peripheral.
 *
 * @param uart UART peripheral identifier.
 * @return Character read from UART data register.
 */
char uart_read_char(hal_uart_t uart)
{
    if (uart == UART1)
        return uart1_read_char();
    else if (uart == UART2)
        return uart2_read_char();
    else if (uart == UART6)
        return uart6_read_char();
    return 0;
}

/**
 * @brief Read a single character from UART1.
 *
 * @return Character read.
 *
 * @note Blocks until a character is available.
 */
char uart1_read_char(void)
{
    while (!(USART1_SR & USART_SR_RXNE))
        ;
    return USART1_DR;
}

/**
 * @brief Read a single character from UART2.
 *
 * @return Character read.
 *
 * @note Blocks until a character is available.
 */
char uart2_read_char(void)
{
    while (!(USART2_SR & USART_SR_RXNE))
        ;
    return USART2_DR;
}

/**
 * @brief Read a single character from UART6.
 *
 * @return Character read.
 *
 * @note Blocks until a character is available.
 */
char uart6_read_char(void)
{
    while (!(USART6_SR & USART_SR_RXNE))
        ;
    return USART6_DR;
}

/**
 * @brief Check if UART peripheral has data available to read.
 *
 * @param uart UART peripheral identifier.
 * @return Non-zero if data available, zero otherwise; -1 if invalid UART.
 */
int uart_available(hal_uart_t uart)
{
    if (uart == UART1)
        return uart1_available();
    else if (uart == UART2)
        return uart2_available();
    else if (uart == UART6)
        return uart6_available();
    return -1;
}

/**
 * @brief Check if UART1 has data available.
 *
 * @return Non-zero if data ready, zero if none.
 */
int uart1_available(void)
{
    return (USART1_SR & USART_SR_RXNE);
}

/**
 * @brief Check if UART2 has data available.
 *
 * @return Non-zero if data ready, zero if none.
 */
int uart2_available(void)
{
    return (USART2_SR & USART_SR_RXNE);
}

/**
 * @brief Check if UART6 has data available.
 *
 * @return Non-zero if data ready, zero if none.
 */
int uart6_available(void)
{
    return (USART6_SR & USART_SR_RXNE);
}

/**
 * @brief Read characters from UART2 into a buffer until delimiter or max length.
 *
 * @param buffer Pointer to buffer to store received characters.
 * @param maxlen Maximum buffer length (including null terminator).
 * @param delimiter Character to stop reading at.
 * @return Number of characters read (excluding null terminator).
 *
 * @note Reads until delimiter is encountered or maxlen-1 characters are read.
 *       Null-terminates the buffer.
 */
uint32_t uart2_read_until(char *buffer, uint32_t maxlen, char delimiter)
{
    uint32_t i = 0;

    while (i < maxlen - 1)
    {
        while (!uart2_available())
            ;

        char c = uart2_read_char();
        if (c == delimiter)
        {
            break;
        }

        buffer[i++] = c;
    }

    buffer[i] = '\0';
    return i;
}
