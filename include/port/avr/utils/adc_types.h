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
 * @brief ADC unit identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single 10-bit ADC, exposed as ::HAL_ADC_0. Its channels
 * ADC0–ADC5 are the Arduino A0–A5 pins on PORTC.
 */

#ifndef ADC_TYPES_H
#define ADC_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC unit identifier (ATmega328P: single ADC).
 */
typedef enum {
  HAL_ADC_0 = 0, /**< The ADC. */
} hal_adc_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ADC_TYPES_H */
