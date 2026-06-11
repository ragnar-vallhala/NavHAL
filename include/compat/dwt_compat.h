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
 * @file dwt_compat.h
 * @brief Deprecated pre-standardization DWT cycle-counter API shim.
 *
 * @details
 * Maps the pre-standardization `dwt_*` names onto the standardized
 * `hal_cycle_counter_*` API as deprecated inline wrappers. Included
 * automatically by `port/cortex-m4/navhal_port_dwt.h`.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 */

#ifndef NAVHAL_DWT_COMPAT_H
#define NAVHAL_DWT_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_cycle_counter_init(). */
NAVHAL_DEPRECATED("use hal_cycle_counter_init")
static inline hal_status_t dwt_init(void) { return hal_cycle_counter_init(); }

/** @deprecated Use hal_cycle_counter_get(). */
NAVHAL_DEPRECATED("use hal_cycle_counter_get")
static inline uint32_t dwt_get_cycles(void) { return hal_cycle_counter_get(); }

/** @deprecated Use hal_cycle_counter_reset(). */
NAVHAL_DEPRECATED("use hal_cycle_counter_reset")
static inline hal_status_t dwt_reset_cycles(void) {
  return hal_cycle_counter_reset();
}

/** @deprecated Use hal_cycle_counter_delay(). */
NAVHAL_DEPRECATED("use hal_cycle_counter_delay")
static inline void dwt_delay_cycles(uint32_t cycles) {
  hal_cycle_counter_delay(cycles);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_DWT_COMPAT_H */
