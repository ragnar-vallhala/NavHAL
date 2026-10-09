/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file hal_dma.h
 * @brief Portable HAL interface for the DMA controller.
 *
 * @details
 * Standardized DMA API (see @c docs/api_standardization.md). Configures and
 * controls DMA streams. The entire API is compiled only when @c NAVHAL_CONFIG_DRV_DMA
 * is defined (see @c NAVHAL_CONFIG_DRV_DMA).
 *
 * ### Typical usage
 * @code
 * hal_dma_config_t cfg = {
 *     .controller = HAL_DMA_CONTROLLER_1,
 *     .stream     = 6,
 *     .channel    = 4,
 *     .direction  = HAL_DMA_DIR_M2P,
 *     .src_addr   = (uint32_t)my_buffer,
 *     .dst_addr   = (uint32_t)&USART2->DR,
 *     .data_count = len,
 *     .src_inc    = 1,
 *     .dst_inc    = 0,
 *     .data_width = HAL_DMA_DATA_WIDTH_8,
 *     .priority   = HAL_DMA_PRIORITY_HIGH,
 * };
 * hal_dma_init(&cfg);
 * hal_dma_start(&cfg);
 * while (!hal_dma_transfer_complete(&cfg));
 * @endcode
 */

#ifndef HAL_DMA_H
#define HAL_DMA_H


/**
 * @defgroup HAL_DMA Dma
 * @ingroup HAL_DRIVERS
 * @brief Direct Memory Access controller.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_DMA

#include "common/hal_status.h"
#include "common/hal_dma_types.h"
#include "common/navhal_compiler.h"
#include <stdbool.h>
#include <stdint.h>








/**
 * @brief Initialize a DMA stream from @p cfg.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */

/**
 * @brief Attach a callback to one DMA stream.
 *
 * The DMA driver owns its stream vectors; this is how anything else -- a
 * driver that moves its data by DMA, or application code -- asks to be told a
 * transfer finished. It replaces reaching past the module with
 * ::hal_interrupt_attach_callback on a DMAx_StreamY IRQ, which worked only
 * because there is exactly one callback slot per IRQ, and so silently unhooked
 * whoever held it first.
 *
 * Attaching the callback already attached is a no-op, so a driver may re-arm
 * per transfer. Attaching a *different* one without detaching returns
 * ::HAL_ERR_BUSY rather than taking the stream: two owners of one stream is a
 * configuration error, and finding out at attach time beats finding out when
 * the other owner's transfer never completes. In-tree, i2c claims DMA1 streams
 * 0 and 5, and sdio claims DMA2 streams 3 and 6.
 *
 * @param controller DMA controller.
 * @param stream     Stream index [0..7].
 * @param cb         Callback, non-NULL.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a bad stream or a NULL callback,
 *         ::HAL_ERR_BUSY if a different callback is already attached.
 */
hal_status_t hal_dma_attach_callback(hal_dma_controller_t controller,
                                     uint8_t stream, hal_dma_callback_t cb);

/**
 * @brief Release a stream's callback.
 * @return ::HAL_OK (also when nothing was attached), ::HAL_ERR_INVALID_ARG for
 *         a bad stream.
 */
hal_status_t hal_dma_detach_callback(hal_dma_controller_t controller,
                                     uint8_t stream);

hal_status_t hal_dma_init(const hal_dma_config_t *cfg);

/**
 * @brief Enable a previously initialized DMA stream.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_start(const hal_dma_config_t *cfg);

/**
 * @brief Disable a DMA stream immediately.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_stop(const hal_dma_config_t *cfg);

/**
 * @brief Check whether the transfer-complete flag is set.
 * @return true if the transfer is complete, false otherwise.
 */
bool hal_dma_transfer_complete(const hal_dma_config_t *cfg);

/**
 * @brief Clear all interrupt flags for the configured stream.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_dma_clear_flags(const hal_dma_config_t *cfg);

/**
 * @brief Re-point an already-initialised stream at a new buffer.
 *
 * Lets a driver start another transfer without reconfiguring the stream.
 * Waits for any in-flight transfer on that stream to finish first.
 *
 * @param cfg   Stream identity (controller/stream); addresses are taken from
 *              @p addr and @p count rather than from @p cfg.
 * @param addr  Memory address for the transfer.
 * @param count Number of data items.
 */
hal_status_t hal_dma_set_memory(const hal_dma_config_t *cfg, uint32_t addr,
                                uint16_t count);

/**
 * @brief Data items still outstanding on a stream.
 *
 * Counts down to zero as the transfer proceeds, so a circular-buffer consumer
 * derives its write index as @c count @c - @c remaining.
 *
 * @param cfg       Stream identity.
 * @param remaining Out-parameter, must not be NULL.
 */
hal_status_t hal_dma_remaining(const hal_dma_config_t *cfg,
                               uint16_t *remaining);

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization DMA type names. Retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED.
 * -------------------------------------------------------------------------- */
typedef hal_dma_controller_t dma_controller_t
    NAVHAL_DEPRECATED("use hal_dma_controller_t");
typedef hal_dma_direction_t dma_direction_t
    NAVHAL_DEPRECATED("use hal_dma_direction_t");
typedef hal_dma_data_width_t dma_data_width_t
    NAVHAL_DEPRECATED("use hal_dma_data_width_t");
typedef hal_dma_priority_t dma_priority_t
    NAVHAL_DEPRECATED("use hal_dma_priority_t");
typedef hal_dma_burst_t dma_burst_t NAVHAL_DEPRECATED("use hal_dma_burst_t");
typedef hal_dma_fifo_threshold_t dma_fifo_threshold_t
    NAVHAL_DEPRECATED("use hal_dma_fifo_threshold_t");
typedef hal_dma_config_t dma_config_t NAVHAL_DEPRECATED("use hal_dma_config_t");

#endif /* NAVHAL_CONFIG_DRV_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: register map, deprecated-function compat shim. */
#if NAVHAL_CONFIG_DRV_DMA
#include "navhal_port_dma.h"

#if defined(NAVHAL_PORT_DMA_COMPAT)
/* Static inline wrappers over the API above: they cannot be defined before
 * the functions they forward to are declared. */
#include "compat/dma_compat.h"
#endif
#endif


/** @} */ /* end of group HAL_DMA */
#endif /* HAL_DMA_H */
