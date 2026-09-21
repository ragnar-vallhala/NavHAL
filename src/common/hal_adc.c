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
 * @file common/hal_adc.c
 * @brief Shared public ADC layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_adc_* API. Argument checks
 * live here so every port agrees on them, and the backends keep only the
 * register work.
 */

#include "common/hal_adc.h"
#include "internal/hal_adc_ops.h"

#include <stddef.h>

/* NULL config is legal here and means "native resolution defaults" -- see
 * hal_adc.h. Unlike every other driver's init, it is not an error, so the
 * shared layer passes it through untouched. */
hal_status_t hal_adc_init(hal_adc_t adc, const hal_adc_config_t *config) {
  return _hal_adc_ops.init(adc, config);
}

hal_status_t hal_adc_read(hal_adc_t adc, uint8_t channel, uint16_t *out) {
  if (out == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_adc_ops.read(adc, channel, out);
}
