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
 * The **data cache** is intentionally left out of this phase. Turning it on
 * breaks the "D-cache off ⇒ DMA buffers are coherent for free" assumption the
 * DMA/SDIO drivers currently rely on, so it requires a clean/invalidate
 * maintenance API and a retrofit of every shared-buffer path — a separate,
 * carefully-validated change.
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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_CACHE */
#endif /* HAL_CACHE_H */
