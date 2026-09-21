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
 * @file common/hal_wwdg.c
 * @brief Shared public window-watchdog layer.
 *
 * @details
 * Validates once and dispatches to ::_hal_wwdg_ops. Guarded to match the
 * backend: the TEST build globs every common/hal_*.c regardless of Kconfig,
 * so this layer disappears with the table it dispatches through.
 */

#include "common/hal_watchdog.h"
#include "internal/hal_wwdg_ops.h"

#if NAVHAL_CONFIG_DRV_WWDG

hal_status_t hal_wwdg_start(uint32_t timeout_ms, uint32_t window_ms) {
  /* Preserves the backend's existing rule: the window must be strictly
   * narrower than the timeout, or the counter leaves the window only after
   * the reset has already fired. */
  if (timeout_ms == 0u || window_ms >= timeout_ms)
    return HAL_ERR_INVALID_ARG;
  return _hal_wwdg_ops.start(timeout_ms, window_ms);
}

hal_status_t hal_wwdg_kick(void) { return _hal_wwdg_ops.kick(); }

bool hal_wwdg_window_open(void) { return _hal_wwdg_ops.window_open(); }

bool hal_wwdg_is_running(void) { return _hal_wwdg_ops.is_running(); }

#endif /* NAVHAL_CONFIG_DRV_WWDG */
