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
 * @file hal_cache.h
 * @brief Portable HAL interface for the Cortex-M7 L1 caches.
 *
 * @details
 * The Cortex-M7 adds L1 instruction and data caches that the Cortex-M4 does
 * not have, so this capability is gated by @c NAVHAL_CONFIG_DRV_CACHE and is
 * offered only on the M7 (`depends on ARCH_CORTEX_M7` in Kconfig). The driver
 * lives at @c src/arch/armv7e-m/cache/cache.c.
 *
 * **Phase 1 — instruction cache only.** Enabling the I-cache is hazard-free:
 * instruction memory is read-only at runtime, so there is no coherency work to
 * do. It is a pure performance win — without it, code executes from flash with
 * wait states (heavy at the F767's 216 MHz). Enable it as early as possible
 * (before ::hal_clock_init) so the whole boot path runs cached:
 *
 * @code
 * #if NAVHAL_CONFIG_DRV_CACHE
 *   hal_icache_enable();   // first thing in main()
 * #endif
 * @endcode
 *
 * **Phase 2 — data cache.** The D-cache is a coherency hazard, not a free win:
 * with it on, CPU accesses go through the cache while DMA engines hit SRAM
 * directly. Memory a DMA reads must be *cleaned* (::hal_dcache_clean) first;
 * memory a DMA writes must be *invalidated* (::hal_dcache_invalidate) before the
 * CPU reads it. Buffers must be ::NAVHAL_CACHE_LINE aligned and size-padded so a
 * range invalidate never discards a neighbour sharing the line.
 *
 * Not every buffer needs maintenance: the Cortex-M7 DTCM is not cached, so a DMA
 * buffer there is coherent for free — @c navhal_dma_mem_class classifies an
 * address so a driver can skip maintenance for DTCM, reject un-DMA-able ITCM,
 * and maintain only cached SRAM. Maintenance ops on an uncached address are
 * hardware no-ops, so calling them unconditionally is also safe.
 */

#ifndef HAL_CACHE_H
#define HAL_CACHE_H

/**
 * @defgroup HAL_CACHE Cache
 * @ingroup HAL_DRIVERS
 * @brief Cortex-M7 L1 cache control.
 * @{
 */

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The per-buffer DMA memory classifier (navhal_dma_mem_class) and its
 * coherency helpers are *not* here: they need the chip's TCM memory map, which
 * is not portable. They live in the per-arch DMA port header — see
 * include/port/cortex-m7/navhal_port_dma.h — while this header keeps only the
 * portable maintenance primitives below.
 */

/**
 * @brief Invalidate and enable the L1 instruction cache.
 *
 * Idempotent and hazard-free (instruction memory is read-only), so it may be
 * called as the very first step of @c main(), before the clock is configured.
 *
 * @return ::HAL_OK once the I-cache is enabled (barriers ensure it is live).
 */
hal_status_t hal_icache_enable(void);

/**
 * @brief Disable and invalidate the L1 instruction cache.
 * @return ::HAL_OK once the I-cache is disabled.
 */
hal_status_t hal_icache_disable(void);

/**
 * @brief Whether the L1 instruction cache is currently enabled.
 * @return @c true if @c SCB_CCR.IC is set.
 */
bool hal_icache_is_enabled(void);

/**
 * @brief Invalidate the whole D-cache, then enable it.
 *
 * The full invalidate (by set/way) clears any stale power-on lines before the
 * cache goes live. Call *after* the MPU is configured and enabled, so the
 * region attributes that govern cacheability are already in force. Not
 * idempotent-cheap like the I-cache — enable it once during board bring-up.
 *
 * @return ::HAL_OK once the D-cache is enabled.
 */
hal_status_t hal_dcache_enable(void);

/**
 * @brief Clean, invalidate, then disable the L1 data cache.
 * @return ::HAL_OK once the D-cache is disabled.
 */
hal_status_t hal_dcache_disable(void);

/**
 * @brief Whether the L1 data cache is currently enabled.
 * @return @c true if @c SCB_CCR.DC is set.
 */
bool hal_dcache_is_enabled(void);

#if NAVHAL_CONFIG_DRV_CACHE

/**
 * @brief Clean (flush) a buffer from the D-cache to main memory.
 *
 * Call before a memory→peripheral DMA so the engine reads the CPU's latest
 * writes. Operates on whole cache lines spanning [@p addr, @p addr + @p size);
 * the caller must ::NAVHAL_DMA_ALIGN the buffer and pad its size. A no-op in
 * hardware for addresses that are not cached (e.g. DTCM).
 *
 * @param addr Buffer start. @param size Buffer length in bytes.
 * @return ::HAL_OK.
 */
hal_status_t hal_dcache_clean(const void *addr, size_t size);

/**
 * @brief Invalidate a buffer's D-cache lines so the CPU re-reads main memory.
 *
 * Call after a peripheral→memory DMA, before the CPU reads the result. Discards
 * cached copies of whole lines spanning the range — if the buffer is not
 * cache-line aligned/padded this destroys neighbouring data, hence the
 * ::NAVHAL_DMA_ALIGN contract. A no-op in hardware for uncached addresses.
 *
 * @param addr Buffer start. @param size Buffer length in bytes.
 * @return ::HAL_OK.
 */
hal_status_t hal_dcache_invalidate(void *addr, size_t size);

/**
 * @brief Clean then invalidate a buffer's D-cache lines.
 *
 * For bidirectional buffers (e.g. a descriptor the CPU writes and the DMA
 * updates). Same alignment contract as ::hal_dcache_invalidate.
 *
 * @param addr Buffer start. @param size Buffer length in bytes.
 * @return ::HAL_OK.
 */
hal_status_t hal_dcache_clean_invalidate(void *addr, size_t size);

#else /* !NAVHAL_CONFIG_DRV_CACHE — no data cache: maintenance is a no-op */

NAVHAL_INLINE hal_status_t hal_dcache_clean(const void *addr, size_t size) {
  (void)addr;
  (void)size;
  return HAL_OK;
}
NAVHAL_INLINE hal_status_t hal_dcache_invalidate(void *addr, size_t size) {
  (void)addr;
  (void)size;
  return HAL_OK;
}
NAVHAL_INLINE hal_status_t hal_dcache_clean_invalidate(void *addr, size_t size) {
  (void)addr;
  (void)size;
  return HAL_OK;
}

#endif /* NAVHAL_CONFIG_DRV_CACHE */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_CACHE */
#endif /* HAL_CACHE_H */
