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
 * @file internal/hal_pwm_ops.h
 * @brief HAL-internal PWM vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_pwm.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_pwm.c) dispatches through. See @c internal/hal_gpio_ops.h
 * for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_PWM_OPS_H
#define NAVHAL_INTERNAL_HAL_PWM_OPS_H

#include "common/hal_pwm.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend PWM operations table. */
typedef struct {
  /** Backend for ::hal_pwm_init. NULL handle / zero frequency rejected upstream. */
  hal_status_t (*init)(hal_pwm_handle_t *pwm, uint32_t frequency,
                       float duty_cycle);
  /** Backend for ::hal_pwm_start. NULL handle rejected upstream. */
  hal_status_t (*start)(hal_pwm_handle_t *pwm);
  /** Backend for ::hal_pwm_stop. NULL handle rejected upstream. */
  hal_status_t (*stop)(hal_pwm_handle_t *pwm);
  /** Backend for ::hal_pwm_set_duty_cycle. NULL handle rejected upstream. */
  hal_status_t (*set_duty_cycle)(hal_pwm_handle_t *pwm, float duty_cycle);
  /** Backend for ::hal_pwm_set_frequency. NULL handle rejected upstream. */
  hal_status_t (*set_frequency)(hal_pwm_handle_t *pwm, uint32_t frequency);
} hal_pwm_ops_t;

/** @brief The active port's PWM operations table (defined by one backend). */
extern const hal_pwm_ops_t _hal_pwm_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_PWM_OPS_H */
