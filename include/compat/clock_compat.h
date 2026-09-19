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
 * @file compat/clock_compat.h
 * @brief Deprecated two-argument clock init shim.
 *
 * @details
 * ::hal_clock_init used to take the PLL configuration as a second argument,
 * which forced a PLL-shaped struct into the portable header and left every
 * port without a PLL accepting an argument it ignored. The PLL parameters now
 * live inside the port's own @c hal_clock_config_t, so init takes one
 * argument and matches the `hal_<p>_init(const hal_<p>_config_t *)` contract
 * every other driver follows.
 *
 * ::hal_clock_init_pll keeps the old call shape working on ports that have a
 * PLL. It is implemented on top of the new signature. New code MUST set
 * @c cfg->pll and call ::hal_clock_init directly.
 *
 * Included automatically by the port's clock header.
 */

#ifndef NAVHAL_CLOCK_COMPAT_H
#define NAVHAL_CLOCK_COMPAT_H

#include "common/hal_clock.h"
#include "common/hal_status.h"
#include "common/navhal_compiler.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_HAS_CLOCK_PLL

/**
 * @deprecated Set @c cfg->pll and call hal_clock_init(cfg) instead.
 *
 * @param cfg     Clock configuration. Its @c pll member is overwritten by
 *                @p pll_cfg when that is non-NULL.
 * @param pll_cfg PLL parameters, or NULL to use whatever @p cfg already holds.
 */
NAVHAL_DEPRECATED("set cfg->pll and use hal_clock_init(cfg)")
static inline hal_status_t hal_clock_init_pll(const hal_clock_config_t *cfg,
                                              const hal_pll_config_t *pll_cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;

  hal_clock_config_t merged = *cfg;
  if (pll_cfg != NULL)
    merged.pll = *pll_cfg;

  return hal_clock_init(&merged);
}

#endif /* NAVHAL_HAS_CLOCK_PLL */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_CLOCK_COMPAT_H */
