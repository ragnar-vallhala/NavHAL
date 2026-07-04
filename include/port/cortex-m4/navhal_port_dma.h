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
 * @file port/cortex-m4/navhal_port_dma.h
 * @brief Cortex-M4 / STM32F4 DMA port header.
 *
 * @details
 * The public DMA API lives in @c common/hal_dma.h, which includes this
 * header. This file carries the STM32F4 DMA register map and the
 * deprecated-function-name compat shim. The entire body is compiled only
 * when @c NAVHAL_CONFIG_DRV_DMA is defined.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_DMA

#include "family/dma_reg.h"

/* Deprecated pre-standardization function names — retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED. */
#include "compat/dma_compat.h"

/* DMA memory classifier + coherency helpers — no-ops on the Cortex-M4, which
 * has neither an L1 data cache nor tightly-coupled memory. The signatures match
 * the M7 port (include/port/cortex-m7/navhal_port_dma.h) so shared driver code
 * (e.g. sdio.c) calls them unconditionally and pays nothing here. */
#include "common/navhal_compiler.h"
#include <stddef.h>

typedef enum {
  NAVHAL_DMA_MEM_ITCM,
  NAVHAL_DMA_MEM_DTCM,
  NAVHAL_DMA_MEM_CACHED,
} navhal_dma_mem_t;

NAVHAL_INLINE navhal_dma_mem_t navhal_dma_mem_class(const void *addr) {
  (void)addr;
  return NAVHAL_DMA_MEM_CACHED;
}
NAVHAL_INLINE hal_status_t navhal_dma_tx_prepare(const void *buf, size_t n) {
  (void)buf;
  (void)n;
  return HAL_OK;
}
NAVHAL_INLINE hal_status_t navhal_dma_rx_guard(const void *buf) {
  (void)buf;
  return HAL_OK;
}
NAVHAL_INLINE void navhal_dma_rx_finish(void *buf, size_t n) {
  (void)buf;
  (void)n;
}

#endif /* NAVHAL_CONFIG_DRV_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_DMA_H */
