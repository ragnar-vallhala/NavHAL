/**
 * @file sdio_compat.h
 * @brief Deprecated pre-standardization SDIO API shim.
 *
 * @details
 * Maps the pre-standardization `sdio_*` function names onto the standardized
 * `hal_sdio_*` API as deprecated inline wrappers. Included automatically by
 * `port/cortex-m4/navhal_port_sdio.h`.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_SDIO_COMPAT_H
#define NAVHAL_SDIO_COMPAT_H

#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_sdio_set_callback(). */
NAVHAL_DEPRECATED("use hal_sdio_set_callback")
static inline void sdio_set_callback(hal_sdio_callback_t callback) {
  hal_sdio_set_callback(callback);
}

/** @deprecated Use hal_sdio_init(). */
NAVHAL_DEPRECATED("use hal_sdio_init")
static inline hal_sdio_error_t sdio_init(const hal_sdio_config_t *config) {
  return hal_sdio_init(config);
}

/** @deprecated Use hal_sdio_card_init(). */
NAVHAL_DEPRECATED("use hal_sdio_card_init")
static inline hal_sdio_error_t sdio_card_init(void) {
  return hal_sdio_card_init();
}

/** @deprecated Use hal_sdio_send_command(). */
NAVHAL_DEPRECATED("use hal_sdio_send_command")
static inline hal_sdio_error_t sdio_send_command(uint8_t cmd_index,
                                                 uint32_t argument,
                                                 uint32_t wait_resp) {
  return hal_sdio_send_command(cmd_index, argument, wait_resp);
}

/** @deprecated Use hal_sdio_get_response(). */
NAVHAL_DEPRECATED("use hal_sdio_get_response")
static inline uint32_t sdio_get_response(uint8_t response_reg) {
  return hal_sdio_get_response(response_reg);
}

/** @deprecated Use hal_sdio_wait_flag(). */
NAVHAL_DEPRECATED("use hal_sdio_wait_flag")
static inline hal_sdio_error_t sdio_wait_flag(uint32_t flag, uint32_t timeout) {
  return hal_sdio_wait_flag(flag, timeout);
}

/** @deprecated Use hal_sdio_wait_sync(). */
NAVHAL_DEPRECATED("use hal_sdio_wait_sync")
static inline hal_sdio_error_t sdio_wait_sync(hal_sdio_error_t result) {
  return hal_sdio_wait_sync(result);
}

/** @deprecated Use hal_sdio_read_block(). */
NAVHAL_DEPRECATED("use hal_sdio_read_block")
static inline hal_sdio_error_t sdio_read_block(uint32_t addr,
                                               uint8_t *buffer) {
  return hal_sdio_read_block(addr, buffer);
}

/** @deprecated Use hal_sdio_write_block(). */
NAVHAL_DEPRECATED("use hal_sdio_write_block")
static inline hal_sdio_error_t sdio_write_block(uint32_t addr,
                                                const uint8_t *buffer) {
  return hal_sdio_write_block(addr, buffer);
}

/** @deprecated Use hal_sdio_get_sector_count(). */
NAVHAL_DEPRECATED("use hal_sdio_get_sector_count")
static inline uint32_t sdio_get_sector_count(void) {
  return hal_sdio_get_sector_count();
}

#ifdef _DMA_ENABLED
/** @deprecated Use hal_sdio_read_block_async(). */
NAVHAL_DEPRECATED("use hal_sdio_read_block_async")
static inline hal_sdio_error_t sdio_read_block_async(uint32_t addr,
                                                     uint8_t *buffer) {
  return hal_sdio_read_block_async(addr, buffer);
}

/** @deprecated Use hal_sdio_write_block_async(). */
NAVHAL_DEPRECATED("use hal_sdio_write_block_async")
static inline hal_sdio_error_t sdio_write_block_async(uint32_t addr,
                                                      const uint8_t *buffer) {
  return hal_sdio_write_block_async(addr, buffer);
}

/** @deprecated Use hal_sdio_read_blocks_async(). */
NAVHAL_DEPRECATED("use hal_sdio_read_blocks_async")
static inline hal_sdio_error_t
sdio_read_blocks_async(uint32_t addr, uint8_t *buffer, uint32_t count) {
  return hal_sdio_read_blocks_async(addr, buffer, count);
}

/** @deprecated Use hal_sdio_write_blocks_async(). */
NAVHAL_DEPRECATED("use hal_sdio_write_blocks_async")
static inline hal_sdio_error_t
sdio_write_blocks_async(uint32_t addr, const uint8_t *buffer, uint32_t count) {
  return hal_sdio_write_blocks_async(addr, buffer, count);
}
#endif /* _DMA_ENABLED */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_SDIO_COMPAT_H */
