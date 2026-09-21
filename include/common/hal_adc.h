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
 * @file hal_adc.h
 * @brief Portable HAL interface for analog-to-digital conversion.
 *
 * @details
 * Standardized ADC API. All public functions use the @c hal_adc_ prefix, take
 * the ADC unit as their first argument (typed ::hal_adc_t), and return
 * ::hal_status_t. This phase covers **blocking, polled, single-channel**
 * conversion — one sample of one regular channel per call.
 *
 * The result from ::hal_adc_read is the raw right-aligned code. Its width is
 * the unit's resolution: 12-bit (0..4095) on the STM32 parts (configurable down
 * to 6-bit), 10-bit (0..1023) on the ATmega328P. The analog input pin must be
 * put in analog mode by the caller (`hal_gpio_set_mode(pin,
 * HAL_GPIO_MODE_ANALOG, ...)`) before the first conversion; the channel number
 * passed here is the ADC's own channel index for that pin (see each board.h).
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

/**
 * @defgroup HAL_ADC ADC
 * @ingroup HAL_DRIVERS
 * @brief Analog-to-digital converter.
 * @{
 */

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/adc_types.h" /* port-resolved ::hal_adc_t instance enum */
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Conversion resolution.
 *
 * The enum values match the STM32 @c ADC_CR1.RES field encoding. The ATmega328P
 * ADC is fixed at 10-bit and ignores this (::HAL_ADC_RES_10BIT is its native
 * width).
 */
typedef enum {
  HAL_ADC_RES_12BIT = 0, /**< 12-bit */
  HAL_ADC_RES_10BIT = 1, /**< 10-bit */
  HAL_ADC_RES_8BIT = 2,  /**< 8-bit. */
  HAL_ADC_RES_6BIT = 3,  /**< 6-bit. */
} hal_adc_resolution_t;

/**
 * @brief ADC unit configuration.
 */
typedef struct {
  hal_adc_resolution_t resolution; /**< Conversion width  */
} hal_adc_config_t;

/**
 * @brief Initialize an ADC unit for polled single-channel conversion.
 *
 * Enables the peripheral clock, selects the conversion resolution, and powers
 * the converter on. Call once per unit before ::hal_adc_read.
 *
 * @param adc    ADC unit (::HAL_ADC_1.. on STM32, ::HAL_ADC_0 on AVR).
 * @param config Unit configuration, or @c NULL for defaults (native resolution).
 * @return ::HAL_OK on success, ::HAL_ERR_INVALID_ARG for an unknown unit.
 */
hal_status_t hal_adc_init(hal_adc_t adc, const hal_adc_config_t *config);

/**
 * @brief Perform one blocking conversion of a single regular channel.
 *
 * @param adc     ADC unit, already ::hal_adc_init'd.
 * @param channel ADC channel index for the analog pin (see the board header).
 * @param out     Receives the raw right-aligned conversion result.
 * @return ::HAL_OK on success, ::HAL_ERR_INVALID_ARG for a null @p out or an
 *         unknown unit, ::HAL_ERR_TIMEOUT if the conversion never completes.
 */
hal_status_t hal_adc_read(hal_adc_t adc, uint8_t channel, uint16_t *out);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_ADC */

#if NAVHAL_CONFIG_DRV_ADC
#include "navhal_port_adc.h" /* port-specific extras (none on the current ports) */
#endif

#endif /* HAL_ADC_H */
