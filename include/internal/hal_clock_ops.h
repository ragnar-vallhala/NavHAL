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
 * @file internal/hal_clock_ops.h
 * @brief HAL-internal clock vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_clock.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_clock.c) dispatches through. See @c internal/hal_gpio_ops.h
 * for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_CLOCK_OPS_H
#define NAVHAL_INTERNAL_HAL_CLOCK_OPS_H

#include "common/hal_clock.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend clock operations table. */
typedef struct {
  /** Backend for ::hal_clock_init. NULL @p cfg already rejected upstream. */
  hal_status_t (*init)(const hal_clock_config_t *cfg,
                       const hal_pll_config_t *pll_cfg);
  /** Backend for ::hal_clock_get_sysclk. */
  uint32_t (*get_sysclk)(void);
  /** Backend for ::hal_clock_get_ahbclk. */
  uint32_t (*get_ahbclk)(void);
  /** Backend for ::hal_clock_get_apb1clk. */
  uint32_t (*get_apb1clk)(void);
  /** Backend for ::hal_clock_get_apb2clk. */
  uint32_t (*get_apb2clk)(void);
} hal_clock_ops_t;

/** @brief The active port's clock operations table (defined by one backend). */
extern const hal_clock_ops_t _hal_clock_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_CLOCK_OPS_H */
