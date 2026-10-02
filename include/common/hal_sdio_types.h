/**
 * @file hal_sdio_types.h
 * @brief SDIO types, split out so the ops table and the port header can
 *        have them without including the API.
 *
 * common/hal_sdio.h ends by including the port header, and everything behind
 * that header -- the family code and the ops table -- needs these types. Taking
 * them from the API header made the include graph circular, which clang-tidy
 * reports at every call site that opens one. Include guards made it compile;
 * they did not make it a graph anyone could follow.
 */
#ifndef HAL_SDIO_TYPES_H
#define HAL_SDIO_TYPES_H

#include "common/hal_config.h" /* NAVHAL_CONFIG_DRV_SDIO: force-included, or pulled here when reachable */
#include <stdbool.h>
#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief How many data lines the bus uses.
 *
 * An enum rather than a count or a flag. The value written to the peripheral is
 * neither: the STM32 WIDBUS field encodes 1-bit as 0, 4-bit as 1 and 8-bit as 2,
 * so a plain lane count would be wrong at every call site and a boolean would
 * have nowhere to put the third case. Naming the widths keeps the mapping in one
 * place and leaves room for 8-bit, which the peripheral supports and SD cards do
 * not -- so it is absent until something needs it.
 */
typedef enum {
  HAL_SDIO_BUS_WIDTH_1BIT = 0, /**< DAT0 only. Always available. */
  HAL_SDIO_BUS_WIDTH_4BIT = 1, /**< DAT0..DAT3, negotiated with ACMD6. */
} hal_sdio_bus_width_t;
/**
 * @brief SDIO operation status / error codes.
 */
typedef enum {
  HAL_SDIO_OK = 0,
  HAL_SDIO_ERROR,
  HAL_SDIO_TIMEOUT,
  HAL_SDIO_CRC_FAIL,
  HAL_SDIO_RX_OVERRUN,
  HAL_SDIO_TX_UNDERRUN,
  HAL_SDIO_PENDING,
  HAL_SDIO_BUSY,
  HAL_SDIO_NO_CARD /**< The slot is empty; see ::hal_sdio_card_present. */
} hal_sdio_error_t;

/**
 * @brief SDIO completion callback type for asynchronous operations.
 */
typedef void (*hal_sdio_callback_t)(hal_sdio_error_t error);

/**
 * @brief SDIO initialization configuration.
 */
typedef struct {
  uint32_t clock_div; /**< SDIO_CK = SDIOCLK / (clock_div + 2). */
  hal_sdio_bus_width_t bus_width; /**< Data lines to use; see the enum. */
} hal_sdio_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_SDIO_TYPES_H */
