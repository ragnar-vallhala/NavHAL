/**
 * @file uart.c
 * @brief UART initialization and I/O functions for STM32F4 UART1, UART2, and
 * UART6.
 *
 * This file provides the implementation of UART communication functions
 * for STM32F401RE microcontroller. It supports USART1, USART2, and USART6
 * peripherals with the following features:
 * - Baud rate configuration
 * - Character, integer, float, and string transmission
 * - Character reception
 * - Data availability checking
 * - Blocking (polling) implementations
 *
 * The implementation uses GPIO alternate functions for UART pins and
 * provides both generic (UART instance parameter) and specific functions.
 *
 * @note All functions are blocking and will wait for hardware operations to
 * complete
 * @note Default configuration: 8 data bits, no parity, 1 stop bit
 *
 * @ingroup HAL_UART
 * @author Ashutosh Vishwakarma
 * @date 2025-07-23
 */

#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/uart_reg.h"

/**
 * @brief Initialize the specified UART peripheral
 *
 * Routes initialization to the appropriate UART-specific init function
 *
 * @param baudrate Desired communication speed in bits per second
 * @param uart     UART instance to initialize (UART1, UART2, or UART6)
 */
void uart_init(uint32_t baudrate, hal_uart_t uart) {
  if (uart == UART2)
    uart2_init(baudrate);
  else if (uart == UART1)
    uart1_init(baudrate);
  else if (uart == UART6)
    uart6_init(baudrate);
}

/**
 * @brief Transmit a single character via the specified UART
 *
 * @param c    Character to transmit
 * @param uart UART instance to use
 */
void uart_write_char(char c, hal_uart_t uart) {
  if (c == '\n') {
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
 *
 * Converts integer to string representation and transmits it
 *
 * @param num  Integer to transmit
 * @param uart UART instance to use
 */
void uart_write_int(int32_t num, hal_uart_t uart) {
  if (uart == UART2)
    uart2_write_int(num);
  else if (uart == UART1)
    uart1_write_int(num);
  else if (uart == UART6)
    uart6_write_int(num);
}

/**
 * @brief Transmit a floating-point number via UART
 *
 * Converts float to string representation with 5 decimal places
 *
 * @param num  Floating-point number to transmit
 * @param uart UART instance to use
 */
void uart_write_float(float num, hal_uart_t uart) {
  if (uart == UART2)
    uart2_write_float(num);
  else if (uart == UART1)
    uart1_write_float(num);
  else if (uart == UART6)
    uart6_write_float(num);
}

/**
 * @brief Transmit a null-terminated string via UART
 *
 * @param s    Null-terminated string to transmit
 * @param uart UART instance to use
 */
void uart_write_string(const char *s, hal_uart_t uart) {
  if (uart == UART2)
    uart2_write_string(s);
  else if (uart == UART1)
    uart1_write_string(s);
  else if (uart == UART6)
    uart6_write_string(s);
}

/**
 * @brief Initialize USART1 peripheral
 *
 * Configures:
 * - GPIO PA9 (TX) as alternate function 7 (USART1_TX)
 * - Baud rate based on APB2 clock
 * - Transmitter enable
 * - USART enable
 *
 * @param baudrate Desired communication speed in bits per second
 */
void uart1_init(uint32_t baudrate) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(1);
  if (usart == NULL)
    return;
  hal_gpio_set_alternate_function(GPIO_PA09, GPIO_AF07);
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  usart->BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) /
               baudrate;      // BRR calculation with rounding
  usart->CR1 |= USART_CR1_TE; // Enable transmitter
  usart->CR1 |= USART_CR1_UE; // Enable USART
}

/**
 * @brief Initialize USART6 peripheral
 *
 * Configures:
 * - GPIO PC6 (TX) as alternate function 7 (USART6_TX)
 * - Baud rate based on APB2 clock
 * - Transmitter enable
 * - USART enable
 *
 * @param baudrate Desired communication speed in bits per second
 */
void uart6_init(uint32_t baudrate) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(6);
  if (usart == NULL)
    return;
  hal_gpio_set_alternate_function(GPIO_PC06, GPIO_AF07);
  RCC->APB2ENR |= RCC_APB2ENR_USART6EN; // Enable USART6 clock
  usart->BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) /
               baudrate;      // BRR calculation with rounding
  usart->CR1 = USART_CR1_TE;  // Enable transmitter
  usart->CR1 |= USART_CR1_UE; // Enable USART
}

/**
 * @brief Initialize USART2 peripheral
 *
 * Configures:
 * - GPIO PA2 (TX) and PA3 (RX) as alternate function 7 (USART2)
 * - Baud rate based on APB1 clock
 * - Transmitter and receiver enable
 * - USART enable
 *
 * @param baudrate Desired communication speed in bits per second
 */
void uart2_init(uint32_t baudrate) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(2);
  if (usart == NULL)
    return;
  hal_gpio_set_alternate_function(GPIO_PA02, GPIO_AF07); // TX
  hal_gpio_set_alternate_function(GPIO_PA03, GPIO_AF07); // RX
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;                  // Enable USART2 clock
  usart->BRR = (hal_clock_get_apb1clk() + (baudrate / 2)) /
               baudrate;                    // BRR calculation with rounding
  usart->CR1 = USART_CR1_TE | USART_CR1_RE; // Enable transmitter and receiver
  usart->CR1 |= USART_CR1_UE;               // Enable USART
}

/**
 * @brief Transmit a single character via USART2
 *
 * Blocks until transmit buffer is empty
 *
 * @param c Character to transmit
 */
void uart2_write_char(char c) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(2);
  if (usart == NULL)
    return;
  if (c == '\n') {
    while (!(usart->SR & USART_SR_TXE))
      ;
    usart->DR = '\r';
  }

  while (!(usart->SR & USART_SR_TXE))
    ;            // Wait until TXE (Transmit Data Register Empty) flag is set
  usart->DR = c; // Write data to data register
}
/**
 * @brief Transmit a single character via USART1
 *
 * @param c Character to transmit
 */
void uart1_write_char(char c) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(1);
  if (usart == NULL)
    return;
  while (!(usart->SR & USART_SR_TXE))
    ;
  usart->DR = c;
}

/**
 * @brief Transmit a single character via USART6
 *
 * @param c Character to transmit
 */
void uart6_write_char(char c) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(6);
  if (usart == NULL)
    return;
  while (!(usart->SR & USART_SR_TXE))
    ;
  usart->DR = c;
}

/**
 * @brief Transmit a 32-bit signed integer via USART2
 *
 * Converts integer to string representation and transmits it
 * Handles negative numbers and zero
 *
 * @param num Integer to transmit
 */
void uart2_write_int(int32_t num) {
  char buf[12]; // Buffer large enough for 32-bit int (-2147483648)
  int i = 0;

  if (num == 0) {
    uart2_write_char('0');
    return;
  }

  if (num < 0) {
    uart2_write_char('-');
    num = -num;
  }

  while (num > 0) {
    buf[i++] = '0' + (num % 10);
    num /= 10;
  }

  while (i--) {
    uart2_write_char(buf[i]); // Write digits in reverse order
  }
}

/**
 * @brief Transmit a 32-bit signed integer via USART1
 *
 * @param num Integer to transmit
 */
void uart1_write_int(int32_t num) {
  char buf[12];
  int i = 0;

  if (num == 0) {
    uart1_write_char('0');
    return;
  }

  if (num < 0) {
    uart1_write_char('-');
    num = -num;
  }

  while (num > 0) {
    buf[i++] = '0' + (num % 10);
    num /= 10;
  }

  while (i--) {
    uart1_write_char(buf[i]);
  }
}

/**
 * @brief Transmit a 32-bit signed integer via USART6
 *
 * @param num Integer to transmit
 */
void uart6_write_int(int32_t num) {
  char buf[12];
  int i = 0;

  if (num == 0) {
    uart6_write_char('0');
    return;
  }

  if (num < 0) {
    uart6_write_char('-');
    num = -num;
  }

  while (num > 0) {
    buf[i++] = '0' + (num % 10);
    num /= 10;
  }

  while (i--) {
    uart6_write_char(buf[i]);
  }
}

/**
 * @brief Transmit a floating-point number via USART2
 *
 * Converts float to string representation with 5 decimal places
 * Handles negative numbers
 *
 * @param num Floating-point number to transmit
 */
void uart2_write_float(float num) {
  if (num < 0) {
    uart2_write_char('-');
    num = -num;
  }
  int32_t integer = (int32_t)num;
  uart2_write_int(integer);
  uart2_write_char('.');
  float floating = num - integer;

  // Write 5 decimal places with rounding
  uart2_write_int((int32_t)(floating * 100000 + 0.5f));
}

/**
 * @brief Transmit a floating-point number via USART1
 *
 * @param num Floating-point number to transmit
 */
void uart1_write_float(float num) {
  if (num < 0) {
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
 * @brief Transmit a floating-point number via USART6
 *
 * @param num Floating-point number to transmit
 */
void uart6_write_float(float num) {
  if (num < 0) {
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
 * @brief Transmit a null-terminated string via USART2
 *
 * @param s Null-terminated string to transmit
 */
void uart2_write_string(const char *s) {
  while (*s) {
    uart2_write_char(*s++);
  }
}

/**
 * @brief Transmit a null-terminated string via USART1
 *
 * @param s Null-terminated string to transmit
 */
void uart1_write_string(const char *s) {
  while (*s) {
    uart1_write_char(*s++);
  }
}

/**
 * @brief Transmit a null-terminated string via USART6
 *
 * @param s Null-terminated string to transmit
 */
void uart6_write_string(const char *s) {
  while (*s) {
    uart6_write_char(*s++);
  }
}

/**
 * @brief Read a single character from the specified UART
 *
 * Blocks until a character is received
 *
 * @param uart UART instance to read from
 * @return Received character
 */
char uart_read_char(hal_uart_t uart) {
  if (uart == UART1)
    return uart1_read_char();
  else if (uart == UART2)
    return uart2_read_char();
  else if (uart == UART6)
    return uart6_read_char();
  return 0;
}

/**
 * @brief Read a single character from USART1
 *
 * @return Received character
 */
char uart1_read_char(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(1);
  if (usart == NULL)
    return 0;
  while (!(usart->SR & USART_SR_RXNE))
    ; // Wait until RXNE (Receive Data Register Not Empty) flag is set
  return usart->DR;
}

/**
 * @brief Read a single character from USART2
 *
 * @return Received character
 */
char uart2_read_char(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(2);
  if (usart == NULL)
    return 0;
  while (!(usart->SR & USART_SR_RXNE))
    ;
  return usart->DR;
}

/**
 * @brief Read a single character from USART6
 *
 * @return Received character
 */
char uart6_read_char(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(6);
  if (usart == NULL)
    return 0;
  while (!(usart->SR & USART_SR_RXNE))
    ;
  return usart->DR;
}

/**
 * @brief Check if data is available to read from specified UART
 *
 * @param uart UART instance to check
 * @return Non-zero if data is available, 0 otherwise
 */
int uart_available(hal_uart_t uart) {
  if (uart == UART1)
    return uart1_available();
  else if (uart == UART2)
    return uart2_available();
  else if (uart == UART6)
    return uart6_available();
  return -1;
}

/**
 * @brief Check if data is available to read from USART1
 *
 * @return Non-zero if data is available, 0 otherwise
 */
int uart1_available(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(1);
  if (usart == NULL)
    return 0;
  return (usart->SR & USART_SR_RXNE);
}

/**
 * @brief Check if data is available to read from USART2
 *
 * @return Non-zero if data is available, 0 otherwise
 */
int uart2_available(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(2);
  if (usart == NULL)
    return 0;
  return (usart->SR & USART_SR_RXNE);
}

/**
 * @brief Check if data is available to read from USART6
 *
 * @return Non-zero if data is available, 0 otherwise
 */
int uart6_available(void) {
  UARTx_Reg_Typedef *usart = GET_USARTx_BASE(6);
  if (usart == NULL)
    return 0;
  return (usart->SR & USART_SR_RXNE);
}

/**
 * @brief Read characters from USART2 until delimiter or max length
 *
 * Reads characters into buffer until:
 * - Delimiter character is received
 * - Max length is reached
 * Always null-terminates the buffer
 *
 * @param buffer     Destination buffer
 * @param maxlen     Maximum number of characters to read (including null
 * terminator)
 * @param delimiter  Character that terminates the read
 * @return Number of characters read (excluding null terminator)
 */
uint32_t uart2_read_until(char *buffer, uint32_t maxlen, char delimiter) {
  uint32_t i = 0;

  while (i < maxlen - 1) {
    while (!uart2_available())
      ; // Wait until a character is available

    char c = uart2_read_char();
    if (c == delimiter) {
      break;
    }

    buffer[i++] = c;
  }

  buffer[i] = '\0'; // Null-terminate
  return i;
}
