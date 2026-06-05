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
 * @file src/common/hal_pwm.c
 * @brief Shared public PWM layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_pwm_* API. It hoists the
 * NULL-handle check (every function) and the zero-frequency check
 * (::hal_pwm_init) that both backends duplicated. Vendor-specific validity
 * (which timers/channels can drive PWM) stays in the backend.
 */

#include "common/hal_pwm.h"
#include "internal/hal_pwm_ops.h"

#include <stddef.h>

hal_status_t hal_pwm_init(hal_pwm_handle_t *pwm, uint32_t frequency,
                          float duty_cycle) {
  if (pwm == NULL || frequency == 0)
    return HAL_ERR_INVALID_ARG;
  return _hal_pwm_ops.init(pwm, frequency, duty_cycle);
}

hal_status_t hal_pwm_start(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_pwm_ops.start(pwm);
}

hal_status_t hal_pwm_stop(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_pwm_ops.stop(pwm);
}

hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t *pwm, float duty_cycle) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_pwm_ops.set_duty_cycle(pwm, duty_cycle);
}

hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t *pwm, uint32_t frequency) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_pwm_ops.set_frequency(pwm, frequency);
}
