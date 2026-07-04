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
 * @file src/vendor/microchip/adc/adc.c
 * @brief ATmega328P ADC HAL driver — blocking 10-bit conversion.
 *
 * @details
 * Implements @c common/hal_adc.h on the ATmega328P's single 10-bit SAR ADC,
 * exposed as ::HAL_ADC_0. Reference is AVcc (with the external cap on AREF);
 * the ADC clock is prescaled by /128 so it lands at 125 kHz on a 16 MHz part —
 * inside the 50–200 kHz the converter needs for full accuracy. The resolution
 * field of ::hal_adc_config_t is ignored (the ATmega ADC is fixed at 10-bit).
 * Conversions are blocking with a coarse iteration-count timeout.
 */

#include "common/hal_adc.h"

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Bounded spin for the conversion wait (far exceeds one 13-cycle conv). */
#define ADC_SPIN 60000U

hal_status_t hal_adc_init(hal_adc_t adc, const hal_adc_config_t *config) {
  (void)config; /* fixed 10-bit */
  if (adc != HAL_ADC_0)
    return HAL_ERR_INVALID_ARG;

  ADMUX = (1u << REFS0); /* AVcc reference, right-adjusted (ADLAR = 0) */
  ADCSRA = (1u << ADEN) | (1u << ADPS2) | (1u << ADPS1) | (1u << ADPS0); /* /128 */
  return HAL_OK;
}

hal_status_t hal_adc_read(hal_adc_t adc, uint8_t channel, uint16_t *out) {
  if (out == NULL || adc != HAL_ADC_0)
    return HAL_ERR_INVALID_ARG;

  ADMUX = (uint8_t)((ADMUX & ~0x0Fu) | (channel & 0x0Fu)); /* select channel */
  ADCSRA |= (1u << ADSC);                                  /* start conversion */

  uint16_t spin = ADC_SPIN;
  while ((ADCSRA & (1u << ADSC)) && spin)
    spin--;
  if (spin == 0u)
    return HAL_ERR_TIMEOUT;

  *out = ADC; /* avr-libc reads ADCL then ADCH in the required order */
  return HAL_OK;
}
