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
#include "core/cortex-m4/config.h"
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

/**
 * @brief Use DMA for UART transmission.
 *
 * @note This option is only available if _DMA_ENABLED is defined.
 */
#ifdef _DMA_ENABLED
#define _UART_BACKEND_DMA
#endif

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
      char: uart_write_char,                                                   \
      signed char: uart_write_int,                                             \
      unsigned char: uart_write_uint,                                          \
      short: uart_write_int,                                                   \
      unsigned short: uart_write_uint,                                         \
      int: uart_write_int,                                                     \
      unsigned int: uart_write_uint,                                           \
      long: uart_write_int,                                                    \
      unsigned long: uart_write_uint,                                          \
      long long: uart_write_int,                                               \
      unsigned long long: uart_write_uint,                                     \
      float: uart_write_float,                                                 \
      double: uart_write_float,                                                \
      const char *: uart_write_string,                                         \
      char *: uart_write_string)(val, UART2)

#define uart1_write(val)                                                       \
  _Generic((val),                                                              \
      char: uart_write_char,                                                   \
      signed char: uart_write_int,                                             \
      unsigned char: uart_write_uint,                                          \
      short: uart_write_int,                                                   \
      unsigned short: uart_write_uint,                                         \
      int: uart_write_int,                                                     \
      unsigned int: uart_write_uint,                                           \
      long: uart_write_int,                                                    \
      unsigned long: uart_write_uint,                                          \
      long long: uart_write_int,                                               \
      unsigned long long: uart_write_uint,                                     \
      float: uart_write_float,                                                 \
      double: uart_write_float,                                                \
      const char *: uart_write_string,                                         \
      char *: uart_write_string)(val, UART1)

#define uart6_write(val)                                                       \
  _Generic((val),                                                              \
      char: uart_write_char,                                                   \
      signed char: uart_write_int,                                             \
      unsigned char: uart_write_uint,                                          \
      short: uart_write_int,                                                   \
      unsigned short: uart_write_uint,                                         \
      int: uart_write_int,                                                     \
      unsigned int: uart_write_uint,                                           \
      long: uart_write_int,                                                    \
      unsigned long: uart_write_uint,                                          \
      long long: uart_write_int,                                               \
      unsigned long long: uart_write_uint,                                     \
      float: uart_write_float,                                                 \
      double: uart_write_float,                                                \
      const char *: uart_write_string,                                         \
      char *: uart_write_string)(val, UART6)

/**
 * @brief Truly generic UART write macro.
 *
 * Dispatches to the correct write function based on value type AND UART
 * instance.
 *
 * Example:
 * @code
 * hal_uart_write(42, UART2);      // Calls uart_write_int(42, UART2)
 * hal_uart_write("Hi", UART6);   // Calls uart_write_string("Hi", UART6)
 * @endcode
 */
#define hal_uart_write(val, uart)                                              \
  _Generic((val),                                                              \
      char: uart_write_char,                                                   \
      signed char: uart_write_int,                                             \
      unsigned char: uart_write_uint,                                          \
      short: uart_write_int,                                                   \
      unsigned short: uart_write_uint,                                         \
      int: uart_write_int,                                                     \
      unsigned int: uart_write_uint,                                           \
      long: uart_write_int,                                                    \
      unsigned long: uart_write_uint,                                          \
      long long: uart_write_int,                                               \
      unsigned long long: uart_write_uint,                                     \
      float: uart_write_float,                                                 \
      double: uart_write_float,                                                \
      const char *: uart_write_string,                                         \
      char *: uart_write_string)(val, uart)

/**
 * @brief Default UART write macro (targets UART2).
 */
#define uart_write(val) uart2_write(val)

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
 * @brief Transmit a 32-bit unsigned integer via UART.
 *
 * @param num  Unsigned integer to transmit.
 * @param uart UART instance to use.
 */
void uart_write_uint(uint32_t num, hal_uart_t uart);

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

/**
 * @brief Transmit a raw byte buffer via the specified UART (polling).
 *
 * @param data   Pointer to the byte buffer.
 * @param length Number of bytes to transmit.
 * @param uart   UART instance to use.
 */
void uart_write_buf(const uint8_t *data, uint16_t length, hal_uart_t uart);

/* USART1 specific functions/macros */
void uart1_init(uint32_t baudrate);
#define uart1_write_char(c) uart_write_char(c, UART1)
#define uart1_write_int(n) uart_write_int(n, UART1)
#define uart1_write_uint(n) uart_write_uint(n, UART1)
#define uart1_write_float(n) uart_write_float(n, UART1)
#define uart1_write_string(s) uart_write_string(s, UART1)
#define uart1_available() uart_available(UART1)
#define uart1_read_char() uart_read_char(UART1)
#define uart1_read_until(b, m, d) uart_read_until(b, m, d, UART1)

/* USART2 specific functions/macros */

/**
 * @brief Initialize USART2 peripheral.
 */
void uart2_init(uint32_t baudrate);
#define uart2_write_char(c) uart_write_char(c, UART2)
#define uart2_write_int(n) uart_write_int(n, UART2)
#define uart2_write_uint(n) uart_write_uint(n, UART2)
#define uart2_write_float(n) uart_write_float(n, UART2)
#define uart2_write_string(s) uart_write_string(s, UART2)
#define uart2_available() uart_available(UART2)
#define uart2_read_char() uart_read_char(UART2)
#define uart2_read_until(b, m, d) uart_read_until(b, m, d, UART2)

/* USART6 specific functions/macros */

/**
 * @brief Initialize USART6 peripheral.
 */
void uart6_init(uint32_t baudrate);
#define uart6_write_char(c) uart_write_char(c, UART6)
#define uart6_write_int(n) uart_write_int(n, UART6)
#define uart6_write_uint(n) uart_write_uint(n, UART6)
#define uart6_write_float(n) uart_write_float(n, UART6)
#define uart6_write_string(s) uart_write_string(s, UART6)
#define uart6_available() uart_available(UART6)
#define uart6_read_char() uart_read_char(UART6)
#define uart6_read_until(b, m, d) uart_read_until(b, m, d, UART6)

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

/**
 * @brief Read characters from UART until a delimiter is found or max_len is
 * reached.
 *
 * @param buffer    Pointer to the buffer to store received data.
 * @param maxlen    Maximum number of bytes to read into the buffer.
 * @param delimiter Character to stop reading at.
 * @param uart      UART instance to read from.
 * @return Number of characters read.
 */
uint32_t uart_read_until(char *buffer, uint32_t maxlen, char delimiter,
                         hal_uart_t uart);

/** @} */ // end of UART_API

/*---------------------------------------------------------------------------
 * DMA-backed UART TX — only available when _DMA_ENABLED and
 * _UART_BACKEND_DMA are both defined.
 *---------------------------------------------------------------------------*/
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

/**
 * @brief Transmit a raw byte buffer over USART2 using DMA.
 *
 * Blocking: returns only after the DMA transfer is complete and the
 * UART shift register has finished clocking out the last byte.
 *
 * USART2_TX maps to DMA1 Stream6 Channel4 on STM32F4.
 *
 * @param data   Pointer to source buffer (must stay valid until return).
 * @param length Number of bytes to transmit.
 *
 * @ingroup HAL_UART
 */
void uart2_write_dma(const uint8_t *data, uint16_t length);

/**
 * @brief Initialize USART1 for DMA-based reception.
 *
 * Sets up the DMA2 Stream 2 Channel 4 in circular mode to fill the provided
 * buffer.
 *
 * @param buffer Destination buffer for received bytes.
 * @param length Size of the buffer.
 */
void uart1_init_dma_rx(uint8_t *buffer, uint16_t length, uint32_t baudrate);

/**
 * @brief Transmit a null-terminated string over USART2 using DMA.
 *
 * Convenience wrapper around uart2_write_dma().
 *
 * @param s Null-terminated string (must stay valid until return).
 *
 * @ingroup HAL_UART
 */
void uart2_write_string_dma(const char *s);

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */

#endif // !CORTEX_M4_UART_H
