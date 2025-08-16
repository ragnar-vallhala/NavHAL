/**
 * @file uart.c
 * @brief UART driver implementation for STM32F4 series
 * @details
 * This file provides UART communication functions for STM32F4 microcontrollers.
 * It supports USART1, USART2, and USART6 peripherals with:
 * - Baud rate configuration
 * - Character, integer, float, and string transmission
 * - Character reception
 * - Data availability checking
 * - Blocking (polling) implementations
 *
 * The implementation uses GPIO alternate functions for UART pins and provides
 * both generic (UART instance parameter) and specific functions.
 *
 * @note All functions are blocking and will wait for hardware operations to complete
 * @note Default configuration: 8 data bits, no parity, 1 stop bit
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/gpio.h"

/**
 * @brief Initialize the specified UART peripheral
 * @param[in] baudrate Desired communication speed in bits per second
 * @param[in] uart UART instance to initialize (UART1, UART2, or UART6)
 * @details Routes initialization to the appropriate UART-specific init function
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
 * @brief Transmit a single character via UART
 * @param[in] c Character to transmit
 * @param[in] uart UART instance to use
 * @details Automatically adds carriage return when newline is transmitted
 */
void uart_write_char(char c, hal_uart_t uart)
{
  if (c == '\n')
  {
    if (uart == UART2)
      uart2_write_char('\r');
    else if (uart == UART1)
      uart1_write_char('\r');
    else if (uart == UART6)
      uart6_write_char('\r');
  }
  if (uart == UART2)
    uart2_write_char(c);
  else if (uart == UART1)
    uart1_write_char(c);
  else if (uart == UART6)
    uart6_write_char(c);
}

/**
 * @brief Transmit a 32-bit signed integer via UART
 * @param[in] num Integer to transmit
 * @param[in] uart UART instance to use
 * @details Converts integer to string representation and transmits it
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
 * @brief Transmit a floating-point number via UART
 * @param[in] num Floating-point number to transmit
 * @param[in] uart UART instance to use
 * @details Converts float to string representation with 5 decimal places
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
 * @brief Transmit a null-terminated string via UART
 * @param[in] s Null-terminated string to transmit
 * @param[in] uart UART instance to use
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

/* UART1 specific functions */

/**
 * @brief Initialize USART1 peripheral
 * @param[in] baudrate Desired communication speed in bits per second
 * @details Configures:
 * - GPIO PA9 (TX) as alternate function 7 (USART1_TX)
 * - Baud rate based on APB2 clock
 * - Transmitter enable
 * - USART enable
 */
void uart1_init(uint32_t baudrate)
{
  hal_gpio_set_alternate_function(GPIO_PA09, GPIO_AF07);
  RCC_APB2ENR |= RCC_APB2ENR_USART1EN; // Enable USART1 clock
  USART1_BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) /
               baudrate;      // BRR calculation with rounding
  USART1_CR1 = USART_CR1_TE;  // Enable transmitter
  USART1_CR1 |= USART_CR1_UE; // Enable USART
}

/**
 * @brief Transmit a single character via USART1
 * @param[in] c Character to transmit
 * @details Blocks until transmit buffer is empty
 */
void uart1_write_char(char c)
{
  while (!(USART1_SR & USART_SR_TXE))
    ;
  USART1_DR = c;
}

/* ... (remaining UART1 functions with similar documentation) ... */

/**
 * @brief Read a single character from the specified UART
 * @param[in] uart UART instance to read from
 * @return Received character
 * @details Blocks until a character is received
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
 * @brief Check if data is available to read from specified UART
 * @param[in] uart UART instance to check
 * @return Non-zero if data is available, 0 otherwise
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
 * @brief Read characters from USART2 until delimiter or max length
 * @param[out] buffer Destination buffer
 * @param[in] maxlen Maximum number of characters to read (including null terminator)
 * @param[in] delimiter Character that terminates the read
 * @return Number of characters read (excluding null terminator)
 * @details
 * - Reads characters into buffer until delimiter or max length
 * - Always null-terminates the buffer
 * - Blocks while waiting for characters
 */
uint32_t uart2_read_until(char *buffer, uint32_t maxlen, char delimiter)
{
  uint32_t i = 0;

  while (i < maxlen - 1)
  {
    while (!uart2_available())
      ; // Wait until a character is available

    char c = uart2_read_char();
    if (c == delimiter)
    {
      break;
    }

    buffer[i++] = c;
  }

  buffer[i] = '\0'; // Null-terminate
  return i;
}