/**
 * @file hal_uart_types.h
 * @brief UART types, split out so the ops table and the port header can
 *        have them without including the API.
 *
 * common/hal_uart.h ends by including the port header, and everything behind
 * that header -- the family code and the ops table -- needs these types. Taking
 * them from the API header made the include graph circular, which clang-tidy
 * reports at every call site that opens one. Include guards made it compile;
 * they did not make it a graph anyone could follow.
 */
#ifndef HAL_UART_TYPES_H
#define HAL_UART_TYPES_H

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_UART_TYPES_H */
