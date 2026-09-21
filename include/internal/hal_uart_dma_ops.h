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
 * @file internal/hal_uart_dma_ops.h
 * @brief HAL-internal UART-over-DMA vendor backend interface.
 *
 * @details
 * A sibling of ::hal_uart_ops_t rather than optional entries inside it. DMA
 * support is a separate driver (@c DRV_UART_DMA, Cortex-M only), so a port
 * either supplies this whole table or none of it -- which keeps the
 * build-time completeness check that NULL-able entries would destroy. Same
 * shape as @c DRV_WWDG.
 *
 * Only two things here are vendor-specific. Everything the public
 * @c hal_uart_*_dma calls do beyond them -- building a transfer descriptor,
 * re-arming, deriving a circular-buffer index, measuring a string -- is
 * portable and lives in @c src/common/hal_uart_dma.c.
 */

#ifndef NAVHAL_INTERNAL_HAL_UART_DMA_OPS_H
#define NAVHAL_INTERNAL_HAL_UART_DMA_OPS_H

#include "common/hal_dma.h"
#include "common/hal_status.h"
#include "common/hal_uart.h"

#include <stdbool.h>

#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port UART-over-DMA operations table. */
typedef struct {
  /**
   * How this UART's TX (@p tx true) or RX endpoint is wired to the DMA
   * controller. The one fact the shared layer cannot derive.
   */
  hal_status_t (*binding)(hal_uart_t uart, bool tx, hal_dma_binding_t *out);
  /** Enable or disable the USART's own DMA request line (CR3 DMAT/DMAR). */
  hal_status_t (*set_request)(hal_uart_t uart, bool tx, bool on);
} hal_uart_dma_ops_t;

/** @brief The active port's UART DMA backend. */
extern const hal_uart_dma_ops_t _hal_uart_dma_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA */

#endif /* NAVHAL_INTERNAL_HAL_UART_DMA_OPS_H */
