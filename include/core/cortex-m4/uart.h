/**
 * @file uart.h
 * @brief UART driver interface for STM32F4 (Cortex-M4).
 *
 * This header defines memory-mapped registers, macros, and function prototypes
 * for UART (USART1, USART2, USART6) peripheral control on the STM32F401RE MCU.
 *
 * The implementation provides:
 * - UART initialization with configurable baud rates
 * - Character, integer, float, and string transmission
 * - Character reception
 * - Data availability checking
 * - Type-generic write macros for simplified usage
 * - Support for all three available UART peripherals (USART1, USART2, USART6)
 *
 * @note Architecture-specific for STM32F401RE. Included through dispatcher
 * based on `CORTEX_M4`.
 * @note All UART functions are blocking (polling mode) implementations.
 *
 * @ingroup HAL_UART
 * @author Ashutosh Vishwakarma
 * @date 2025-07-23
 */

#ifndef CORTEX_M4_UART_H
#define CORTEX_M4_UART_H
#include "core/cortex-m4/uart_reg.h"
#include <stdint.h>
/**
 * @brief UART identifier enumeration.
 *
 * Identifies which UART peripheral to use for operations.
 */
typedef enum {
  UART1 = 1, ///< USART1 - APB2 peripheral, typically higher speed
  UART2 = 2, ///< USART2 - APB1 peripheral
  UART6 = 6  ///< USART6 - APB2 peripheral
} hal_uart_t;

/** @defgroup UART_WRITE_MACROS Type-Generic Write Macros
 *  @brief Simplified macros to write different types to UART using `_Generic`.
 *
 *  These macros provide a type-safe interface that automatically selects the
 *  appropriate write function based on the argument type. Supports:
 *  - All integer types (char, short, int, long)
 *  - Floating point types (float, double)
 *  - String types (char *, const char *)
 *
 *  Example:
 *  @code
 *  uart2_write(42);       // Calls uart2_write_int
 *  uart2_write(3.14f);    // Calls uart2_write_float
 *  uart2_write("hello");  // Calls uart2_write_string
 *  @endcode
 *  @{
 */

#define uart2_write(val)                                                       \
  _Generic((val),                                                              \
      char: uart2_write_char,                                                  \
      signed char: uart2_write_int,                                            \
      unsigned char: uart2_write_int,                                          \
      short: uart2_write_int,                                                  \
      unsigned short: uart2_write_int,                                         \
      int: uart2_write_int,                                                    \
      unsigned int: uart2_write_int,                                           \
      long: uart2_write_int,                                                   \
      unsigned long: uart2_write_int,                                          \
      long long: uart2_write_int,                                              \
      unsigned long long: uart2_write_int,                                     \
      float: uart2_write_float,                                                \
      double: uart2_write_float,                                               \
      const char *: uart2_write_string,                                        \
      char *: uart2_write_string)(val)

#define uart1_write(val)                                                       \
  _Generic((val),                                                              \
      char: uart1_write_char,                                                  \
      signed char: uart1_write_int,                                            \
      unsigned char: uart1_write_int,                                          \
      short: uart1_write_int,                                                  \
      unsigned short: uart1_write_int,                                         \
      int: uart1_write_int,                                                    \
      unsigned int: uart1_write_int,                                           \
      long: uart1_write_int,                                                   \
      unsigned long: uart1_write_int,                                          \
      long long: uart1_write_int,                                              \
      unsigned long long: uart1_write_int,                                     \
      float: uart1_write_float,                                                \
      double: uart1_write_float,                                               \
      const char *: uart1_write_string,                                        \
      char *: uart1_write_string)(val)

#define uart6_write(val)                                                       \
  _Generic((val),                                                              \
      char: uart6_write_char,                                                  \
      signed char: uart6_write_int,                                            \
      unsigned char: uart6_write_int,                                          \
      short: uart6_write_int,                                                  \
      unsigned short: uart6_write_int,                                         \
      int: uart6_write_int,                                                    \
      unsigned int: uart6_write_int,                                           \
      long: uart6_write_int,                                                   \
      unsigned long: uart6_write_int,                                          \
      long long: uart6_write_int,                                              \
      unsigned long long: uart6_write_int,                                     \
      float: uart6_write_float,                                                \
      double: uart6_write_float,                                               \
      const char *: uart6_write_string,                                        \
      char *: uart6_write_string)(val)

/** @} */ // end of UART_WRITE_MACROS

/** @defgroup UART_API UART HAL API
 *  @brief UART driver API functions.
 *
 *  Provides initialization, transmission, and reception functions for UART
 * peripherals. Both generic (UART instance parameter) and specific
 * (UART1/UART2/UART6) functions are provided for flexibility.
 *  @{
 */

/**
 * @brief Initialize the specified UART peripheral.
 *
 * Configures the UART peripheral with the specified baud rate and enables
 * both transmitter and receiver. Also enables the peripheral clock.
 *
 * @param baudrate Desired communication speed in bits per second.
 * @param uart     UART instance to initialize (UART1, UART2, or UART6).
 */
void uart_init(uint32_t baudrate, hal_uart_t uart);

/**
 * @brief Transmit a single character via the specified UART.
 *
 * Blocks until the transmit buffer is empty before sending.
 *
 * @param c    Character to transmit.
 * @param uart UART instance to use.
 */
void uart_write_char(char c, hal_uart_t uart);

/**
 * @brief Transmit a 32-bit signed integer via UART.
 *
 * Converts the integer to its string representation and transmits it.
 *
 * @param num  Integer to transmit.
 * @param uart UART instance to use.
 */
void uart_write_int(int32_t num, hal_uart_t uart);

/**
 * @brief Transmit a floating-point number via UART.
 *
 * Converts the float to its string representation with default formatting
 * and transmits it.
 *
 * @param num  Floating-point number to transmit.
 * @param uart UART instance to use.
 */
void uart_write_float(float num, hal_uart_t uart);

/**
 * @brief Transmit a null-terminated string via UART.
 *
 * Transmits each character of the string until the null terminator.
 *
 * @param s    Null-terminated string to transmit.
 * @param uart UART instance to use.
 */
void uart_write_string(const char *s, hal_uart_t uart);

/* USART1 specific functions */

/**
 * @brief Initialize USART1 peripheral.
 *
 * @param baudrate Desired communication speed in bits per second.
 */
void uart1_init(uint32_t baudrate);

/**
 * @brief Transmit a single character via USART1.
 */
void uart1_write_char(char c);

/**
 * @brief Transmit a 32-bit signed integer via USART1.
 */
void uart1_write_int(int32_t num);

/**
 * @brief Transmit a floating-point number via USART1.
 */
void uart1_write_float(float num);

/**
 * @brief Transmit a null-terminated string via USART1.
 */
void uart1_write_string(const char *s);

/* USART2 specific functions */

/**
 * @brief Initialize USART2 peripheral.
 */
void uart2_init(uint32_t baudrate);
void uart2_write_char(char c);
void uart2_write_int(int32_t num);
void uart2_write_float(float num);
void uart2_write_string(const char *s);

/* USART6 specific functions */

/**
 * @brief Initialize USART6 peripheral.
 */
void uart6_init(uint32_t baudrate);
void uart6_write_char(char c);
void uart6_write_int(int32_t num);
void uart6_write_float(float num);
void uart6_write_string(const char *s);

/**
 * @brief Read a single character from the specified UART.
 *
 * Blocks until a character is received.
 *
 * @param uart UART instance to read from.
 * @return Received character.
 */
char uart_read_char(hal_uart_t uart);

/**
 * @brief Check if data is available to read.
 *
 * @param uart UART instance to check.
 * @return 1 if data is available, 0 otherwise.
 */
int uart_available(hal_uart_t uart);

/* USART1 specific receive functions */
int uart1_available(void);
char uart1_read_char(void);

/* USART2 specific receive functions */
int uart2_available(void);
char uart2_read_char(void);

/* USART6 specific receive functions */
int uart6_available(void);
char uart6_read_char(void);

/**
 * @brief Read characters into a buffer until a delimiter is found or max length
 * is reached.
 *
 * Reads characters into the provided buffer until either:
 * - The specified delimiter character is received
 * - The maximum buffer length is reached
 * - A timeout occurs
 *
 * The delimiter character is not included in the buffer. The buffer is always
 * null-terminated.
 *
 * @param buffer     Destination buffer for received characters.
 * @param maxlen     Maximum number of characters to read (including null
 * terminator).
 * @param delimiter  Character that terminates the read operation.
 * @return Number of characters read (excluding null terminator).
 */
uint32_t uart2_read_until(char *buffer, uint32_t maxlen, char delimiter);

/** @} */ // end of UART_API

#endif // !CORTEX_M4_UART_H
