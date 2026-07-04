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
 * @file adc_types.h
 * @brief ADC unit identifier — Cortex-M7 / STM32F7 port.
 *
 * @details
 * The set of valid ADC units is target-defined, so ::hal_adc_t is
 * port-resolved. The STM32F767ZI has three 12-bit ADCs (ADC1/2/3); the
 * Nucleo-144 Arduino analog header routes A0–A2 to ADC1 and A3–A5 to ADC3.
 */

#ifndef ADC_TYPES_H
#define ADC_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC unit identifier (STM32F767ZI: ADC1/2/3).
 */
typedef enum {
  HAL_ADC_1 = 0, /**< ADC unit 1. */
  HAL_ADC_2 = 1, /**< ADC unit 2. */
  HAL_ADC_3 = 2, /**< ADC unit 3. */
} hal_adc_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ADC_TYPES_H */
