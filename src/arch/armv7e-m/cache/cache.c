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
 * @file cache.c
 * @brief Standardized HAL L1-cache driver for the Cortex-M7.
 *
 * @details
 * Implements the `hal_*cache_*` API from `common/hal_cache.h` against the
 * Cortex-M7 System Control Block. The instruction cache is a hazard-free,
 * pure-performance enable (instruction memory is read-only, so there is no
 * coherency work). The data cache adds the coherency-maintenance primitives —
 * a full invalidate/clean by set/way for enable/disable, and clean/invalidate
 * by MVA for the per-buffer DMA paths. The whole unit is gated by
 * NAVHAL_CONFIG_DRV_CACHE, which Kconfig offers only on ARCH_CORTEX_M7.
 */

#include "navhal_port_config.h"
#if NAVHAL_CONFIG_DRV_CACHE

#include "common/hal_cache.h"
#include <stdint.h>

/* System Control Block cache controls (Cortex-M7). */
#define SCB_CCR    (*(volatile uint32_t *)0xE000ED14UL) /* Config + Control    */
#define SCB_CCSIDR (*(volatile uint32_t *)0xE000ED80UL) /* Cache Size ID       */
#define SCB_CSSELR (*(volatile uint32_t *)0xE000ED84UL) /* Cache Size Select   */
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50UL) /* I-cache inval all   */
#define SCB_DCIMVAC (*(volatile uint32_t *)0xE000EF5CUL) /* D-cache inval by MVA */
#define SCB_DCISW   (*(volatile uint32_t *)0xE000EF60UL) /* D-cache inval by S/W */
#define SCB_DCCMVAC (*(volatile uint32_t *)0xE000EF68UL) /* D-cache clean by MVA */
#define SCB_DCCIMVAC (*(volatile uint32_t *)0xE000EF70UL) /* D clean+inval by MVA */
#define SCB_DCCISW  (*(volatile uint32_t *)0xE000EF74UL) /* D clean+inval by S/W */

#define SCB_CCR_IC (1UL << 17) /* Instruction-cache enable */
#define SCB_CCR_DC (1UL << 16) /* Data-cache enable        */

static inline void cache_barrier(void) {
  __asm volatile("dsb 0xF" ::: "memory");
  __asm volatile("isb 0xF" ::: "memory");
}

hal_status_t hal_icache_enable(void) {
  cache_barrier();
  SCB_ICIALLU = 0UL; /* invalidate the whole I-cache to the point of unification */
  cache_barrier();
  SCB_CCR |= SCB_CCR_IC;
  cache_barrier();
  return HAL_OK;
}

hal_status_t hal_icache_disable(void) {
  cache_barrier();
  SCB_CCR &= ~SCB_CCR_IC;
  SCB_ICIALLU = 0UL; /* invalidate so stale lines cannot be used after re-enable */
  cache_barrier();
  return HAL_OK;
}

bool hal_icache_is_enabled(void) { return (SCB_CCR & SCB_CCR_IC) != 0UL; }

/* ------------------------------------------------------------------ *
 * Data cache
 * ------------------------------------------------------------------ */

/**
 * @brief Walk every set/way of the L1 D-cache, writing @p sw_reg per line.
 *
 * Drives either DCISW (invalidate) or DCCISW (clean+invalidate). Set and way
 * field positions are derived from CCSIDR so this is independent of the actual
 * cache size (F767: 16 KB, 4-way, 32-byte line).
 */
static void dcache_walk_sets_ways(volatile uint32_t *sw_reg) {
  SCB_CSSELR = 0UL; /* select level-1 data cache */
  cache_barrier();

  uint32_t ccsidr = SCB_CCSIDR;
  uint32_t set_shift = (ccsidr & 7UL) + 4UL;        /* log2(line bytes)         */
  uint32_t ways = (ccsidr >> 3) & 0x3FFUL;          /* associativity - 1        */
  uint32_t way_shift = (uint32_t)__builtin_clz(ways); /* MSB-justified way field */
  uint32_t sets = (ccsidr >> 13) & 0x7FFFUL;        /* number of sets - 1       */

  for (;;) {
    uint32_t w = ways;
    for (;;) {
      *sw_reg = (sets << set_shift) | (w << way_shift);
      if (w-- == 0UL)
        break;
    }
    if (sets-- == 0UL)
      break;
  }
  cache_barrier();
}

/** @brief Apply an MVA maintenance op (@p mva_reg) to every cache line in range. */
static void dcache_walk_range(volatile uint32_t *mva_reg, uintptr_t addr,
                              size_t size) {
  if (size == 0U)
    return;
  uintptr_t line = NAVHAL_CACHE_LINE;
  uintptr_t p = addr & ~(line - 1U); /* round start down to a line boundary */
  uintptr_t end = addr + size;
  __asm volatile("dsb 0xF" ::: "memory");
  while (p < end) {
    *mva_reg = (uint32_t)p;
    p += line;
  }
  __asm volatile("dsb 0xF" ::: "memory");
  __asm volatile("isb 0xF" ::: "memory");
}

hal_status_t hal_dcache_enable(void) {
  if ((SCB_CCR & SCB_CCR_DC) != 0UL)
    return HAL_OK; /* already on — a second invalidate would drop live data */
  dcache_walk_sets_ways(&SCB_DCISW); /* drop any stale power-on lines */
  SCB_CCR |= SCB_CCR_DC;
  cache_barrier();
  return HAL_OK;
}

hal_status_t hal_dcache_disable(void) {
  __asm volatile("dsb 0xF" ::: "memory");
  SCB_CCR &= ~SCB_CCR_DC;
  dcache_walk_sets_ways(&SCB_DCCISW); /* flush dirty lines out on the way down */
  return HAL_OK;
}

bool hal_dcache_is_enabled(void) { return (SCB_CCR & SCB_CCR_DC) != 0UL; }

hal_status_t hal_dcache_clean(const void *addr, size_t size) {
  dcache_walk_range(&SCB_DCCMVAC, (uintptr_t)addr, size);
  return HAL_OK;
}

hal_status_t hal_dcache_invalidate(void *addr, size_t size) {
  dcache_walk_range(&SCB_DCIMVAC, (uintptr_t)addr, size);
  return HAL_OK;
}

hal_status_t hal_dcache_clean_invalidate(void *addr, size_t size) {
  dcache_walk_range(&SCB_DCCIMVAC, (uintptr_t)addr, size);
  return HAL_OK;
}

#endif /* NAVHAL_CONFIG_DRV_CACHE */
