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
 * @file internal/hal_reset_ops.h
 * @brief HAL-internal reset-cause and system-reset vendor backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API -- application code includes
 * @c common/hal_reset.h. Each port defines exactly one ::_hal_reset_ops with
 * its register work; the shared layer in @c src/common/hal_reset.c validates
 * arguments once and dispatches here.
 *
 * Every entry is a register touch with no portable part: latching the
 * cause flags, reporting them, and asking the core to reset. There is nothing
 * for a shared layer to compute, so this table mirrors the API.
 */

#ifndef NAVHAL_INTERNAL_HAL_RESET_OPS_H
#define NAVHAL_INTERNAL_HAL_RESET_OPS_H

#include "common/hal_reset.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port reset operations table. */
typedef struct {
  /** Backend for ::hal_reset_init -- latch and clear the cause flags. */
  hal_status_t (*init)(void);
  /** Backend for ::hal_reset_get_cause. */
  uint32_t (*get_cause)(void);
  /** Backend for ::hal_system_reset. Does not return on success. */
  hal_status_t (*system_reset)(void);
} hal_reset_ops_t;

/** @brief The active port's reset backend (defined by one backend). */
extern const hal_reset_ops_t _hal_reset_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_RESET_OPS_H */
