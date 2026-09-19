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
 * @file internal/hal_adc_ops.h
 * @brief HAL-internal ADC vendor backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API -- application code includes
 * @c common/hal_adc.h. Each port defines exactly one ::_hal_adc_ops with
 * its register work; the shared layer in @c src/common/hal_adc.c validates
 * arguments once and dispatches here.
 *
 * Both entries are irreducible: configuring the converter and running a
 * single conversion. Multi-channel or averaged reads, if they ever arrive,
 * belong in the shared layer built on ::read.
 */

#ifndef NAVHAL_INTERNAL_HAL_ADC_OPS_H
#define NAVHAL_INTERNAL_HAL_ADC_OPS_H

#include "common/hal_adc.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port adc operations table. */
typedef struct {
  /** Backend for ::hal_adc_init. Validation (NULL @p config) has already run. */
  hal_status_t (*init)(hal_adc_t adc, const hal_adc_config_t *config);
  /** Backend for ::hal_adc_read. @p out is non-NULL. */
  hal_status_t (*read)(hal_adc_t adc, uint8_t channel, uint16_t *out);
} hal_adc_ops_t;

/** @brief The active port's adc backend (defined by one backend). */
extern const hal_adc_ops_t _hal_adc_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_ADC_OPS_H */
