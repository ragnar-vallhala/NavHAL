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
 * @file port/cortex-m7/navhal_port_dma.h
 * @brief Cortex-M7 / STM32F7 DMA port header.
 *
 * @details
 * The public DMA API lives in @c common/hal_dma.h, which includes this
 * header. This file carries the STM32F7 DMA register map, the
 * deprecated-function-name compat shim, and — unique to the M7 — the DMA
 * memory classifier and cache-coherency helpers a driver wraps around a
 * transfer once the L1 D-cache is enabled. The entire body is compiled only
 * when @c NAVHAL_CONFIG_DRV_DMA is defined.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"
#include "common/hal_interrupt.h"


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_DMA

#include "family/dma_reg.h"

/* Deprecated pre-standardization function names — retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED. */
#include "compat/dma_compat.h"

/* ------------------------------------------------------------------ *
 * DMA memory classifier + coherency helpers (general-purpose DMA1/DMA2)
 *
 * With the L1 D-cache on, a buffer the CPU shares with a DMA engine needs
 * clean/invalidate maintenance — unless it lives in DTCM, which the DMA1/DMA2
 * controllers reach but the cache never holds. ITCM is reachable by no DMA at
 * all. The classifier tells these apart; the helpers apply the right action so
 * drivers (I2C/UART/SPI/SDIO) stay coherency-agnostic. On the M4/AVR ports the
 * same helpers exist as no-ops, so shared driver code calls them unconditionally.
 *
 * TCM base addresses are Cortex-M7 architectural; the sizes come from the board
 * (Kconfig -> NAVHAL_CONFIG_*_SIZE). A cache-on board that omits them fails to
 * build here rather than mis-classifying silently.
 * ------------------------------------------------------------------ */

#include "common/hal_cache.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Cortex-M7 instruction-TCM window base (architectural). */
#define NAVHAL_ITCM_BASE 0x00000000UL
/** @brief Cortex-M7 data-TCM window base (architectural). */
#define NAVHAL_DTCM_BASE 0x20000000UL

/** @brief How a buffer relates to general-purpose DMA coherency. */
typedef enum {
  NAVHAL_DMA_MEM_ITCM,   /**< Instruction TCM: unreachable by any DMA — reject. */
  NAVHAL_DMA_MEM_DTCM,   /**< Data TCM: DMA1/2-reachable, uncached — no maintenance. */
  NAVHAL_DMA_MEM_CACHED, /**< SRAM/Flash: cacheable — clean/invalidate as needed. */
} navhal_dma_mem_t;

#if NAVHAL_CONFIG_DRV_CACHE

#if !defined(NAVHAL_CONFIG_ITCM_SIZE) || !defined(NAVHAL_CONFIG_DTCM_SIZE)
#error "D-cache enabled but the board did not declare ITCM_SIZE/DTCM_SIZE "       \
       "(add them to the board Kconfig); the DMA memory classifier needs the "    \
       "TCM extents."
#endif

/** @brief Classify a buffer address for DMA cache handling. */
NAVHAL_INLINE navhal_dma_mem_t navhal_dma_mem_class(const void *addr) {
  uintptr_t a = (uintptr_t)addr;
  /* Offset form (a - base < size) is a single-compare range test that also
   * dodges the "unsigned >= 0 is always true" warning when a base is 0. */
  if ((uintptr_t)(a - NAVHAL_ITCM_BASE) < (uintptr_t)NAVHAL_CONFIG_ITCM_SIZE)
    return NAVHAL_DMA_MEM_ITCM;
  if ((uintptr_t)(a - NAVHAL_DTCM_BASE) < (uintptr_t)NAVHAL_CONFIG_DTCM_SIZE)
    return NAVHAL_DMA_MEM_DTCM;
  return NAVHAL_DMA_MEM_CACHED;
}

#else /* !NAVHAL_CONFIG_DRV_CACHE — no data cache: everything is coherent */

/** @brief Without a D-cache no address needs maintenance — always "cached". */
NAVHAL_INLINE navhal_dma_mem_t navhal_dma_mem_class(const void *addr) {
  (void)addr;
  return NAVHAL_DMA_MEM_CACHED;
}

#endif /* NAVHAL_CONFIG_DRV_CACHE */

/**
 * @brief Prepare a buffer for a memory→peripheral (TX) DMA.
 * @return ::HAL_ERR_INVALID_ARG if the buffer is DMA-unreachable (ITCM), else
 *         ::HAL_OK after cleaning it (a no-op for DTCM / cache-off builds).
 */
NAVHAL_INLINE hal_status_t navhal_dma_tx_prepare(const void *buf, size_t n) {
  switch (navhal_dma_mem_class(buf)) {
  case NAVHAL_DMA_MEM_ITCM:
    return HAL_ERR_INVALID_ARG;
  case NAVHAL_DMA_MEM_DTCM:
    return HAL_OK;
  default:
    return hal_dcache_clean(buf, n);
  }
}

/**
 * @brief Reachability guard for a peripheral→memory (RX) buffer, pre-DMA.
 * @return ::HAL_ERR_INVALID_ARG for an ITCM buffer, else ::HAL_OK.
 */
NAVHAL_INLINE hal_status_t navhal_dma_rx_guard(const void *buf) {
  return (navhal_dma_mem_class(buf) == NAVHAL_DMA_MEM_ITCM) ? HAL_ERR_INVALID_ARG
                                                            : HAL_OK;
}

/**
 * @brief Finish a peripheral→memory (RX) DMA: invalidate so the CPU re-reads.
 * A no-op for DTCM and for cache-off builds.
 */
NAVHAL_INLINE void navhal_dma_rx_finish(void *buf, size_t n) {
  if (navhal_dma_mem_class(buf) == NAVHAL_DMA_MEM_CACHED)
    hal_dcache_invalidate(buf, n);
}

#endif /* NAVHAL_CONFIG_DRV_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif


/**
 * @brief How one peripheral endpoint is wired to the DMA controller.
 *
 * Port-defined, like a clock config: controller/stream/channel is this
 * family's DMA model and would not fit a part with DMAMUX or bus-master DMA.
 * A bus driver's DMA table reports this and the shared layer does the rest.
 */
typedef struct {
  hal_dma_controller_t controller; /**< Which controller drives this endpoint. */
  uint8_t stream;                  /**< Stream index. */
  uint8_t channel;                 /**< Channel selection. */
  uint32_t periph_addr;            /**< Peripheral data register address. */
  hal_irq_t irq;                   /**< Stream completion IRQ. */
} hal_dma_binding_t;

#endif /* NAVHAL_PORT_DMA_H */
