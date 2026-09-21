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
 * @file hal_features.h
 * @brief NavHAL capability macros — historical/compat documentation only.
 *
 * @details
 * NavHAL gates every optional capability on a single family of macros,
 * `NAVHAL_CONFIG_<KCONFIG_SYMBOL>` (e.g. `NAVHAL_CONFIG_DRV_DMA`), which is a
 * 1:1 mirror of the Kconfig `.config`. The generator (`tools/kconfig.py`)
 * emits them into `navhal_target.h`, and the build **force-includes** that
 * header into every translation unit (Linux `-include autoconf.h` style), so
 * the macros are ambient — no `#include` required. Always test with
 * `#if NAVHAL_CONFIG_DRV_<X>`; the macros are always defined as `0`/`1`.
 *
 * @par Deprecated `NAVHAL_HAS_*` contract
 * The older capability names (`NAVHAL_HAS_DMA`, `NAVHAL_HAS_CYCLE_COUNTER`, …)
 * are retained **only** as thin aliases of the matching `NAVHAL_CONFIG_*`
 * macro, generated alongside them in `navhal_target.h` for out-of-tree
 * consumers. New in-tree code MUST use `NAVHAL_CONFIG_DRV_*`. The alias map
 * (including the renames `DRV_DWT`→`CYCLE_COUNTER`, `DRV_CRC`→`CRC_HW`,
 * `USE_FPU`→`FPU`) lives in `NAVHAL_HAS_MAP` in `tools/kconfig.py`.
 *
 * This header no longer defines any macros; it is kept as a documentation
 * anchor and to preserve the include path for legacy consumers.
 */

#ifndef HAL_FEATURES_H
#define HAL_FEATURES_H

/**
 * @defgroup HAL_FEATURES Features
 * @ingroup HAL_CORE
 * @brief Capability macros for portable feature detection.
 * @{
 */

/** @} */ /* end of group HAL_FEATURES */
#endif /* HAL_FEATURES_H */
