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
 * @file internal/hal_wwdg_ops.h
 * @brief HAL-internal window-watchdog vendor backend interface.
 *
 * @details
 * A sibling of ::hal_watchdog_ops_t, not extra entries inside it. The window
 * watchdog is its own Kconfig driver (@c DRV_WWDG) and the ATmega328P has no
 * such peripheral, so a port supplies this whole table or none of it and the
 * build-time completeness check on the base table stays meaningful.
 *
 * Every entry is a register touch. @c window_open reads the live counter
 * against the window value, and @c is_running reflects hardware that cannot
 * be disarmed once started, so neither can be cached by a shared layer.
 */

#ifndef NAVHAL_INTERNAL_HAL_WWDG_OPS_H
#define NAVHAL_INTERNAL_HAL_WWDG_OPS_H

#include "common/hal_status.h"
#include "common/hal_watchdog.h"

#include <stdbool.h>
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_WWDG

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port window-watchdog operations table. */
typedef struct {
  /** Backend for ::hal_wwdg_start. */
  hal_status_t (*start)(uint32_t timeout_ms, uint32_t window_ms);
  /** Backend for ::hal_wwdg_kick. Refusing outside the window is the point. */
  hal_status_t (*kick)(void);
  /** Backend for ::hal_wwdg_window_open. */
  bool (*window_open)(void);
  /** Backend for ::hal_wwdg_is_running. */
  bool (*is_running)(void);
} hal_wwdg_ops_t;

/** @brief The active port's window-watchdog backend. */
extern const hal_wwdg_ops_t _hal_wwdg_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_CONFIG_DRV_WWDG */

#endif /* NAVHAL_INTERNAL_HAL_WWDG_OPS_H */
