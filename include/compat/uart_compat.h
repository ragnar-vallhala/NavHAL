/**
 * @file uart_compat.h
 * @brief Deprecated pre-standardization UART API shim.
 *
 * @details
 * Maps the pre-standardization UART names onto the standardized `hal_uart_*`
 * API. Function-form wrappers are marked deprecated, so legacy calls emit a
 * compiler warning naming the standardized replacement; the type-generic
 * `uartN_write` / `uart_write` macros forward silently (a `_Generic` macro
 * cannot carry an attribute). Included automatically by
 * `port/cortex-m4/navhal_port_uart.h` after the standardized declarations.
 *
 * Note: the pre-standardization functions took the UART instance as their
 * *last* argument; the standardized API takes it *first*. These wrappers
 * reorder accordingly.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_UART_COMPAT_H
#define NAVHAL_UART_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/* ---- Generic (instance-as-argument) legacy functions -------------------- */

/** @deprecated Use hal_uart_init(). */
NAVHAL_DEPRECATED("use hal_uart_init")
static inline hal_status_t uart_init(uint32_t baudrate, hal_uart_t uart) {
  hal_uart_config_t _c = {.baudrate = baudrate};
  return hal_uart_init(uart, &_c);
}

/** @deprecated Use hal_uart_write_char(). */
NAVHAL_DEPRECATED("use hal_uart_write_char")
static inline hal_status_t uart_write_char(char c, hal_uart_t uart) {
  return hal_uart_write_char(uart, c);
}

/** @deprecated Use hal_uart_write_int(). */
NAVHAL_DEPRECATED("use hal_uart_write_int")
static inline hal_status_t uart_write_int(int32_t num, hal_uart_t uart) {
  return hal_uart_write_int(uart, num);
}

/** @deprecated Use hal_uart_write_uint(). */
NAVHAL_DEPRECATED("use hal_uart_write_uint")
static inline hal_status_t uart_write_uint(uint32_t num, hal_uart_t uart) {
  return hal_uart_write_uint(uart, num);
}

/** @deprecated Use hal_uart_write_float(). */
NAVHAL_DEPRECATED("use hal_uart_write_float")
static inline hal_status_t uart_write_float(float num, hal_uart_t uart) {
  return hal_uart_write_float(uart, num);
}

/** @deprecated Use hal_uart_write_string(). */
NAVHAL_DEPRECATED("use hal_uart_write_string")
static inline hal_status_t uart_write_string(const char *s, hal_uart_t uart) {
  return hal_uart_write_string(uart, s);
}

/** @deprecated Use hal_uart_write(). */
NAVHAL_DEPRECATED("use hal_uart_write")
static inline hal_status_t uart_write_buf(const uint8_t *data, uint16_t length,
                                          hal_uart_t uart) {
  return hal_uart_write(uart, data, length);
}

/** @deprecated Use hal_uart_read_char(). */
NAVHAL_DEPRECATED("use hal_uart_read_char")
static inline char uart_read_char(hal_uart_t uart) {
  return hal_uart_read_char(uart);
}

/** @deprecated Use hal_uart_available(). */
NAVHAL_DEPRECATED("use hal_uart_available")
static inline int uart_available(hal_uart_t uart) {
  return hal_uart_available(uart) ? 1 : 0;
}

/** @deprecated Use hal_uart_read_until(). */
NAVHAL_DEPRECATED("use hal_uart_read_until")
static inline uint32_t uart_read_until(char *buffer, uint32_t maxlen,
                                       char delimiter, hal_uart_t uart) {
  return hal_uart_read_until(uart, buffer, maxlen, delimiter);
}

/* ---- Per-instance (uartN_*) legacy functions ---------------------------- */

/*
 * Generates the deprecated uart<N>_* function wrappers for one instance.
 * @param n  Instance digit (1, 2, 6).
 * @param ID Corresponding hal_uart_t value.
 */
#define _NAVHAL_UART_INSTANCE_COMPAT(n, ID)                                    \
  NAVHAL_DEPRECATED("use hal_uart_init")                                       \
  static inline hal_status_t uart##n##_init(uint32_t baud) {                   \
    hal_uart_config_t _c = {.baudrate = baud};                                 \
    return hal_uart_init(ID, &_c);                                             \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_char")                                 \
  static inline hal_status_t uart##n##_write_char(char c) {                    \
    return hal_uart_write_char(ID, c);                                         \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_int")                                  \
  static inline hal_status_t uart##n##_write_int(int32_t num) {                \
    return hal_uart_write_int(ID, num);                                        \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_uint")                                 \
  static inline hal_status_t uart##n##_write_uint(uint32_t num) {              \
    return hal_uart_write_uint(ID, num);                                       \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_float")                                \
  static inline hal_status_t uart##n##_write_float(float num) {                \
    return hal_uart_write_float(ID, num);                                      \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_string")                               \
  static inline hal_status_t uart##n##_write_string(const char *s) {           \
    return hal_uart_write_string(ID, s);                                       \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_available")                                  \
  static inline int uart##n##_available(void) {                                \
    return hal_uart_available(ID) ? 1 : 0;                                     \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_read_char")                                  \
  static inline char uart##n##_read_char(void) {                               \
    return hal_uart_read_char(ID);                                             \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_read_until")                                 \
  static inline uint32_t uart##n##_read_until(char *b, uint32_t m, char d) {   \
    return hal_uart_read_until(ID, b, m, d);                                   \
  }

_NAVHAL_UART_INSTANCE_COMPAT(1, HAL_UART_1)
_NAVHAL_UART_INSTANCE_COMPAT(2, HAL_UART_2)
_NAVHAL_UART_INSTANCE_COMPAT(6, HAL_UART_6)

/* ---- Type-generic write macros (cannot carry a deprecation attribute) --- */

/** @deprecated Use hal_uart_print(HAL_UART_1, val). */
#define uart1_write(val) hal_uart_print(HAL_UART_1, (val))
/** @deprecated Use hal_uart_print(HAL_UART_2, val). */
#define uart2_write(val) hal_uart_print(HAL_UART_2, (val))
/** @deprecated Use hal_uart_print(HAL_UART_6, val). */
#define uart6_write(val) hal_uart_print(HAL_UART_6, (val))
/** @deprecated Use hal_uart_print(HAL_UART_2, val). */
#define uart_write(val) hal_uart_print(HAL_UART_2, (val))

/* ---- DMA-backed legacy API ---------------------------------------------- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

/** @deprecated Use hal_uart_write_dma(). */
NAVHAL_DEPRECATED("use hal_uart_write_dma")
static inline hal_status_t uart_write_dma(const uint8_t *data, uint16_t length,
                                          hal_uart_t uart) {
  return hal_uart_write_dma(uart, data, length);
}

/** @deprecated Use hal_uart_init_dma_rx(). */
NAVHAL_DEPRECATED("use hal_uart_init_dma_rx")
static inline hal_status_t uart_init_dma_rx(uint8_t *buffer, uint16_t length,
                                            hal_uart_t uart) {
  return hal_uart_init_dma_rx(uart, buffer, length);
}

/** @deprecated Use hal_uart_write_string_dma(). */
NAVHAL_DEPRECATED("use hal_uart_write_string_dma")
static inline hal_status_t uart_write_string_dma(const char *s,
                                                 hal_uart_t uart) {
  return hal_uart_write_string_dma(uart, s);
}

/* Per-instance DMA wrappers for one instance. */
#define _NAVHAL_UART_INSTANCE_DMA_COMPAT(n, ID)                                \
  NAVHAL_DEPRECATED("use hal_uart_write_dma")                                  \
  static inline hal_status_t uart##n##_write_dma(const uint8_t *d,             \
                                                 uint16_t len) {              \
    return hal_uart_write_dma(ID, d, len);                                     \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_init_dma_rx")                                \
  static inline hal_status_t uart##n##_init_dma_rx(uint8_t *b, uint16_t len) { \
    return hal_uart_init_dma_rx(ID, b, len);                                   \
  }                                                                            \
  NAVHAL_DEPRECATED("use hal_uart_write_string_dma")                           \
  static inline hal_status_t uart##n##_write_string_dma(const char *s) {       \
    return hal_uart_write_string_dma(ID, s);                                   \
  }

_NAVHAL_UART_INSTANCE_DMA_COMPAT(1, HAL_UART_1)
_NAVHAL_UART_INSTANCE_DMA_COMPAT(2, HAL_UART_2)
_NAVHAL_UART_INSTANCE_DMA_COMPAT(6, HAL_UART_6)

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_UART_COMPAT_H */
