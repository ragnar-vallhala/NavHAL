/**
 * @file port/cortex-m4/navhal_port_sdio.h
 * @brief Cortex-M4 / STM32F4 SDIO port header.
 *
 * @details
 * The public SDIO API lives in @c common/hal_sdio.h, which includes this
 * header. This file carries the SDIO register-bit defines, the SDIO register
 * map include, the asynchronous (DMA-backed) prototypes, and the
 * deprecated-name shim. Like @c navhal_port_dma.h, the whole body is gated by
 * @c _SDIO_ENABLED so it collapses to nothing on a target without SDIO.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_SDIO_H
#define NAVHAL_PORT_SDIO_H

#include "common/hal_sdio.h"
#include "navhal_port_config.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _SDIO_ENABLED

#include "family/sdio_reg.h"

#define SDIO_CMD_WAITPEND (1 << 9)
#define SDIO_CMD_CPSMEN (1 << 10)

#ifdef _DMA_ENABLED
/** @brief Asynchronous (DMA) single-block read. */
hal_sdio_error_t hal_sdio_read_block_async(uint32_t addr, uint8_t *buffer);
/** @brief Asynchronous (DMA) single-block write. */
hal_sdio_error_t hal_sdio_write_block_async(uint32_t addr,
                                            const uint8_t *buffer);
/** @brief Asynchronous (DMA) multi-block read. */
hal_sdio_error_t hal_sdio_read_blocks_async(uint32_t addr, uint8_t *buffer,
                                            uint32_t count);
/** @brief Asynchronous (DMA) multi-block write. */
hal_sdio_error_t hal_sdio_write_blocks_async(uint32_t addr,
                                             const uint8_t *buffer,
                                             uint32_t count);
#endif /* _DMA_ENABLED */

#endif /* _SDIO_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef _SDIO_ENABLED
/* Deprecated pre-standardization SDIO names — retained as a backward-compat
 * alias behind NAVHAL_DEPRECATED. */
#include "compat/sdio_compat.h"
#endif /* _SDIO_ENABLED */

#endif /* NAVHAL_PORT_SDIO_H */
