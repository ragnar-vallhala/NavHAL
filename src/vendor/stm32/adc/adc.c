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
 *
 * Sample time comes from `hal_adc_config_t` and is written per channel on every
 * read (`SMPR1`/`SMPR2`), because the silicon keeps it per channel while the
 * configuration is per unit.
 */

#include "navhal_port_adc.h"
#include "internal/hal_adc_ops.h"

#include "family/adc_reg.h"
#include "family/rcc_reg.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Bounded spin for the end-of-conversion wait. */
#define ADC_SPIN 1000000U

/* hal_adc_sample_time_t -> SMPR field encoding. The public enum is ordered so
 * that 0 is the safe end (see hal_adc.h); the silicon encodes 3 cycles as 0, so
 * the two do not line up and a table is needed rather than a cast. */
static const uint8_t _smp_bits[] = {
    7U, /* HAL_ADC_SAMPLE_DEFAULT   -> 480 cycles */
    0U, /* 3   */
    1U, /* 15  */
    2U, /* 28  */
    3U, /* 56  */
    4U, /* 84  */
    5U, /* 112 */
    6U, /* 144 */
    7U, /* 480 */
};

/* Set per unit at init, applied per channel at read: SMPR is a per-channel
 * field, and the channel is not known until the read. */
static uint8_t _smp[ADC_UNIT_COUNT];

static inline volatile ADC_Reg_Typedef *_adc(hal_adc_t adc) {
  return GET_ADCx_BASE((uint8_t)adc);
}

static hal_status_t stm32_adc_init(hal_adc_t adc, const hal_adc_config_t *config) {
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

  uint32_t smp = (config != NULL) ? (uint32_t)config->sample_time
                                  : (uint32_t)HAL_ADC_SAMPLE_DEFAULT;
  if (smp >= (sizeof(_smp_bits) / sizeof(_smp_bits[0])))
    return HAL_ERR_INVALID_ARG;
  _smp[(uint8_t)adc] = _smp_bits[smp];

  /* Single conversion, right-aligned; power the converter on. */
  a->CR2 = ADC_CR2_ADON;

  /* tSTAB — let the converter settle before the first conversion. */
  for (volatile uint32_t i = 0; i < 10000U; i++)
    ;

  return HAL_OK;
}

static hal_status_t stm32_adc_read(hal_adc_t adc, uint8_t channel, uint16_t *out) {
  if (out == NULL || (uint8_t)adc >= ADC_UNIT_COUNT)
    return HAL_ERR_INVALID_ARG;
  volatile ADC_Reg_Typedef *a = _adc(adc);

  /* Sample time for this channel: SMPR2 holds channels 0-9, SMPR1 channels
   * 10-18, three bits each. Left at the reset value this is 3 cycles, which is
   * correct only for a source of about a hundred ohms -- anything higher reads
   * low, with nothing to say so. */
  const uint32_t smp = (uint32_t)_smp[(uint8_t)adc];
  if (channel < 10U) {
    const uint32_t sh = 3U * (uint32_t)channel;
    a->SMPR2 = (a->SMPR2 & ~(7U << sh)) | (smp << sh);
  } else {
    const uint32_t sh = 3U * ((uint32_t)channel - 10U);
    a->SMPR1 = (a->SMPR1 & ~(7U << sh)) | (smp << sh);
  }

  a->SQR1 = 0U;                     /* L = 0 -> one conversion in the sequence */
  a->SQR3 = (uint32_t)channel & 0x1FU; /* SQ1 = this channel */

  /* Discard any latched EOC before starting. A read that timed out left its
   * conversion running, and it finishes and sets EOC regardless -- so without
   * this, the next read sees EOC already high, skips the wait entirely and
   * returns the abandoned sample as HAL_OK. The status bits are rc_w0: writing
   * 1 to a bit leaves it alone, writing 0 clears it. */
  a->SR &= ~(uint32_t)ADC_SR_EOC;

  a->CR2 |= ADC_CR2_SWSTART;        /* start the regular conversion */
  uint32_t spin = ADC_SPIN;
  while (!(a->SR & ADC_SR_EOC) && spin)
    spin--;
  if (spin == 0U)
    return HAL_ERR_TIMEOUT;

  *out = (uint16_t)(a->DR & 0xFFFFU); /* reading DR clears EOC */
  return HAL_OK;
}

/** @brief The STM32 adc backend. */
const hal_adc_ops_t _hal_adc_ops = {
    .init = stm32_adc_init,
    .read = stm32_adc_read,
};
