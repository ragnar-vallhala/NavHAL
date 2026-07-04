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
 * @file adc.c
 * @brief Standardized HAL ADC driver for STM32F4 / STM32F7.
 *
 * @details
 * Implements the `hal_adc_*` API from `common/hal_adc.h` for the classic STM32
 * SAR ADC, which is register-compatible across the F4 and F7 families — the
 * same driver serves both, selected only by the family `adc_reg.h` (unit count
 * and bases). Blocking, polled, single regular-channel conversion.
 *
 * The ADC clock is prescaled by /4 (`ADC_CCR.ADCPRE`) so ADCCLK stays within the
 * 36 MHz limit across the APB2 frequencies these boards run (F401 ≤ 84 MHz,
 * F767 ≤ 108 MHz). The caller sets the analog pin to `HAL_GPIO_MODE_ANALOG`.
 */

#include "navhal_port_adc.h"

#include "family/adc_reg.h"
#include "family/rcc_reg.h"
#include <stddef.h>

/** @brief Bounded spin for the end-of-conversion wait. */
#define ADC_SPIN 1000000U

static inline volatile ADC_Reg_Typedef *_adc(hal_adc_t adc) {
  return GET_ADCx_BASE((uint8_t)adc);
}

hal_status_t hal_adc_init(hal_adc_t adc, const hal_adc_config_t *config) {
  if ((uint8_t)adc >= ADC_UNIT_COUNT)
    return HAL_ERR_INVALID_ARG;
  volatile ADC_Reg_Typedef *a = _adc(adc);

  /* Peripheral clock — ADC1/2/3EN are consecutive APB2ENR bits. */
  RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN << (uint8_t)adc);

  /* Common prescaler /4 (ADCPRE = 01) keeps ADCCLK <= 36 MHz. */
  ADC_COMMON->CCR =
      (ADC_COMMON->CCR & ~(3U << ADC_CCR_ADCPRE_Pos)) | (1U << ADC_CCR_ADCPRE_Pos);

  uint32_t res = (config != NULL) ? (uint32_t)config->resolution
                                  : (uint32_t)HAL_ADC_RES_12BIT;
  a->CR1 = (a->CR1 & ~ADC_CR1_RES_MASK) |
           ((res << ADC_CR1_RES_Pos) & ADC_CR1_RES_MASK);

  /* Single conversion, right-aligned; power the converter on. */
  a->CR2 = ADC_CR2_ADON;

  /* tSTAB — let the converter settle before the first conversion. */
  for (volatile uint32_t i = 0; i < 10000U; i++)
    ;

  return HAL_OK;
}

hal_status_t hal_adc_read(hal_adc_t adc, uint8_t channel, uint16_t *out) {
  if (out == NULL || (uint8_t)adc >= ADC_UNIT_COUNT)
    return HAL_ERR_INVALID_ARG;
  volatile ADC_Reg_Typedef *a = _adc(adc);

  a->SQR1 = 0U;                     /* L = 0 -> one conversion in the sequence */
  a->SQR3 = (uint32_t)channel & 0x1FU; /* SQ1 = this channel */

  a->CR2 |= ADC_CR2_SWSTART;        /* start the regular conversion */
  uint32_t spin = ADC_SPIN;
  while (!(a->SR & ADC_SR_EOC) && spin)
    spin--;
  if (spin == 0U)
    return HAL_ERR_TIMEOUT;

  *out = (uint16_t)(a->DR & 0xFFFFU); /* reading DR clears EOC */
  return HAL_OK;
}
