/**
 * @file hal_uart.h
 * @brief Portable HAL interface for UART communication.
 *
 * @details
 * Standardized UART API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_uart_ prefix, take the UART instance as their
 * first argument, and return ::hal_status_t (queries return their value
 * directly). The set of valid instance IDs (e.g. ::HAL_UART_1 / _2 / _6 on
 * the STM32F401RE) is target-defined.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/uart_types.h" /* port-resolved ::hal_uart_t instance enum */
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief UART configuration passed to ::hal_uart_init. */
typedef struct {
  uint32_t baudrate; /**< Baud rate in bits per second. */
} hal_uart_config_t;

/**
 * @brief Initialize a UART peripheral (8N1, transmitter + receiver enabled).
 * @param uart UART instance.
 * @param cfg  Configuration; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL config / invalid UART.
 */
hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg);

/**
 * @brief Enable/disable a UART's RX/TX interrupts (peripheral + NVIC).
 * @param uart  UART instance.
 * @param rx_en Non-zero to enable the RXNE interrupt.
 * @param tx_en Non-zero to enable the TXE interrupt.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid UART.
 */
hal_status_t hal_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                       uint8_t tx_en);

/**
 * @brief Transmit a raw byte buffer (blocking).
 * @param uart   UART instance.
 * @param data   Source buffer.
 * @param length Number of bytes to transmit.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid UART / NULL buffer.
 */
hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length);

/** @brief Transmit a single character (blocking). */
hal_status_t hal_uart_write_char(hal_uart_t uart, char c);
/** @brief Transmit a 32-bit signed integer as decimal text (blocking). */
hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num);
/** @brief Transmit a 32-bit unsigned integer as decimal text (blocking). */
hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num);
/** @brief Transmit a float as decimal text (blocking). */
hal_status_t hal_uart_write_float(hal_uart_t uart, float num);
/** @brief Transmit a null-terminated string (blocking). */
hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s);

/**
 * @brief Read a single character (blocks until one is received).
 * @param uart UART instance.
 * @return The received character, or 0 on error / invalid UART.
 */
char hal_uart_read_char(hal_uart_t uart);

/**
 * @brief Check whether received data is available.
 * @param uart UART instance.
 * @return true if a byte is waiting in RX, false otherwise.
 */
bool hal_uart_available(hal_uart_t uart);

/**
 * @brief Read characters until a delimiter is seen or @p maxlen-1 is reached.
 * @param uart      UART instance.
 * @param buffer    Destination buffer (null-terminated on return).
 * @param maxlen    Size of @p buffer.
 * @param delimiter Character that stops the read.
 * @return Number of characters stored (excluding the null terminator).
 */
uint32_t hal_uart_read_until(hal_uart_t uart, char *buffer, uint32_t maxlen,
                             char delimiter);

/**
 * @brief Type-generic UART write helper.
 *
 * Dispatches to the appropriate @c hal_uart_write_* function based on the
 * argument type. Replaces the former per-instance @c uartN_write macros.
 *
 * @code
 * hal_uart_print(HAL_UART_2, 42);       // -> hal_uart_write_int
 * hal_uart_print(HAL_UART_2, 3.14f);    // -> hal_uart_write_float
 * hal_uart_print(HAL_UART_2, "hello");  // -> hal_uart_write_string
 * @endcode
 */
#define hal_uart_print(uart, val)                                              \
  _Generic((val),                                                              \
      char: hal_uart_write_char,                                               \
      signed char: hal_uart_write_int,                                         \
      unsigned char: hal_uart_write_uint,                                      \
      short: hal_uart_write_int,                                               \
      unsigned short: hal_uart_write_uint,                                     \
      int: hal_uart_write_int,                                                 \
      unsigned int: hal_uart_write_uint,                                       \
      long: hal_uart_write_int,                                                \
      unsigned long: hal_uart_write_uint,                                      \
      long long: hal_uart_write_int,                                           \
      unsigned long long: hal_uart_write_uint,                                 \
      float: hal_uart_write_float,                                             \
      double: hal_uart_write_float,                                            \
      const char *: hal_uart_write_string,                                     \
      char *: hal_uart_write_string)((uart), (val))

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: DMA-backed API behind @c _DMA_ENABLED, compat shim. */
#include "navhal_port_uart.h"

#endif /* HAL_UART_H */
