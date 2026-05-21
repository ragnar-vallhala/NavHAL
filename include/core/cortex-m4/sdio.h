/**
 * @file core/cortex-m4/sdio.h
 * @brief Cortex-M4 / STM32F4 SDIO port header.
 *
 * @details
 * The public SDIO API lives in @c common/hal_sdio.h, which includes this
 * header. This file carries the SDIO register-bit defines, the SDIO register
 * map include, the asynchronous (DMA-backed) prototypes, and the
 * deprecated-name shim.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_SDIO_H
#define CORTEX_M4_SDIO_H

#include "common/hal_sdio.h"
#include "core/cortex-m4/config.h"
#include "family/sdio_reg.h"

#define SDIO_CMD_WAITPEND (1 << 9)
#define SDIO_CMD_CPSMEN (1 << 10)


#ifdef __cplusplus
extern "C" {
#endif

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
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization SDIO names — removed in M5. */
#include "compat/sdio_compat.h"

#endif /* CORTEX_M4_SDIO_H */
