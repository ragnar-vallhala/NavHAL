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
 * Cortex-M7 System Control Block. Phase 1 covers the instruction cache only —
 * a hazard-free, pure-performance enable (instruction memory is read-only, so
 * there is no coherency maintenance to do). The data cache is deliberately not
 * touched here (see hal_cache.h). The whole unit is gated by
 * NAVHAL_CONFIG_DRV_CACHE, which Kconfig offers only on ARCH_CORTEX_M7.
 */

#include "navhal_port_config.h"
#if NAVHAL_CONFIG_DRV_CACHE

#include "common/hal_cache.h"
#include <stdint.h>

/* System Control Block cache controls (Cortex-M7). */
#define SCB_CCR    (*(volatile uint32_t *)0xE000ED14UL) /* Config + Control    */
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50UL) /* I-cache inval all   */

#define SCB_CCR_IC (1UL << 17) /* Instruction-cache enable */

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

#endif /* NAVHAL_CONFIG_DRV_CACHE */
