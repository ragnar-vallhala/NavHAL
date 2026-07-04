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
 * @file family/adc_reg.h
 * @brief Cortex-M7 / STM32F767 ADC register map and bit masks (RM0410 §15).
 *
 * @details
 * The STM32F767 carries three 12-bit SAR ADCs (ADC1/2/3) on APB2, each with the
 * same register block — register-compatible with the STM32F4 ADC IP, so the
 * shared @c adc.c drives both families. The units sit 0x100 apart and their
 * APB2ENR enables are consecutive bits, so a single 0-based index selects both.
 */

#ifndef CORTEX_M7_ADC_REG_H
#define CORTEX_M7_ADC_REG_H

#include "common/hal_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC peripheral register map (one converter).
 */
typedef struct {
  __IO uint32_t SR;    /**< Status register (0x00). */
  __IO uint32_t CR1;   /**< Control register 1 (0x04). */
  __IO uint32_t CR2;   /**< Control register 2 (0x08). */
  __IO uint32_t SMPR1; /**< Sample-time register 1, channels 10–18 (0x0C). */
  __IO uint32_t SMPR2; /**< Sample-time register 2, channels 0–9 (0x10). */
  __IO uint32_t JOFR1; /**< Injected offset 1 (0x14). */
  __IO uint32_t JOFR2; /**< Injected offset 2 (0x18). */
  __IO uint32_t JOFR3; /**< Injected offset 3 (0x1C). */
  __IO uint32_t JOFR4; /**< Injected offset 4 (0x20). */
  __IO uint32_t HTR;   /**< Watchdog high threshold (0x24). */
  __IO uint32_t LTR;   /**< Watchdog low threshold (0x28). */
  __IO uint32_t SQR1;  /**< Regular sequence 1, length in L[23:20] (0x2C). */
  __IO uint32_t SQR2;  /**< Regular sequence 2 (0x30). */
  __IO uint32_t SQR3;  /**< Regular sequence 3, SQ1[4:0] = 1st channel (0x34). */
  __IO uint32_t JSQR;  /**< Injected sequence (0x38). */
  __IO uint32_t JDR1;  /**< Injected data 1 (0x3C). */
  __IO uint32_t JDR2;  /**< Injected data 2 (0x40). */
  __IO uint32_t JDR3;  /**< Injected data 3 (0x44). */
  __IO uint32_t JDR4;  /**< Injected data 4 (0x48). */
  __IO uint32_t DR;    /**< Regular data register (0x4C). */
} ADC_Reg_Typedef;

/**
 * @brief ADC common registers (prescaler, temp/VREF/VBAT enables).
 */
typedef struct {
  __IO uint32_t CSR; /**< Common status (0x00). */
  __IO uint32_t CCR; /**< Common control (0x04). */
  __IO uint32_t CDR; /**< Common regular data, dual/triple mode (0x08). */
} ADC_Common_Typedef;

/** @brief Number of ADC units on this part. */
#define ADC_UNIT_COUNT 3U
/** @brief ADC1 register base (ADC2 = +0x100, ADC3 = +0x200). */
#define ADC1_BASE 0x40012000UL
/** @brief ADC common-register base (shared by all units). */
#define ADC_COMMON_BASE 0x40012300UL

/** @brief Register pointer for ADC unit index @p idx (0-based: ADC1/2/3). */
#define GET_ADCx_BASE(idx)                                                     \
  ((volatile ADC_Reg_Typedef *)(ADC1_BASE + (0x100UL * (uint32_t)(idx))))
/** @brief The common-register block. */
#define ADC_COMMON ((volatile ADC_Common_Typedef *)ADC_COMMON_BASE)

/** @brief APB2ENR clock-enable bit for ADC1 (units are consecutive: <<idx). */
#define RCC_APB2ENR_ADC1EN (1U << 8)

/* --- SR --- */
#define ADC_SR_EOC (1U << 1) /**< Regular end of conversion. */

/* --- CR1 --- */
#define ADC_CR1_RES_Pos 24U         /**< Resolution field position. */
#define ADC_CR1_RES_MASK (3U << 24) /**< Resolution field mask. */

/* --- CR2 --- */
#define ADC_CR2_ADON (1U << 0)     /**< A/D converter on. */
#define ADC_CR2_ALIGN (1U << 11)   /**< Data alignment (0 = right). */
#define ADC_CR2_SWSTART (1U << 30) /**< Start conversion of the regular group. */

/* --- SQR1 --- */
#define ADC_SQR1_L_Pos 20U /**< Regular sequence length (L = count − 1). */

/* --- CCR (common) --- */
#define ADC_CCR_ADCPRE_Pos 16U /**< Prescaler: 00=/2, 01=/4, 10=/6, 11=/8. */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CORTEX_M7_ADC_REG_H */
