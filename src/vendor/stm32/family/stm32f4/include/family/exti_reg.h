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
 * @file exti_reg.h
 * @brief EXTI controller register map for STM32F4.
 *
 * @details
 * The EXTI sits between an event source and the NVIC: a line that is not
 * unmasked here never reaches the CPU, however the peripheral behind it is
 * configured. Lines 0..15 are the GPIO pins; the rest are internal sources,
 * including the two the RTC uses.
 *
 * Reference: RM0368 §10.
 */

#ifndef CORTEX_M4_EXTI_REG_H
#define CORTEX_M4_EXTI_REG_H

#include "common/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  __IO uint32_t IMR;   /**< 0x00: Interrupt mask */
  __IO uint32_t EMR;   /**< 0x04: Event mask */
  __IO uint32_t RTSR;  /**< 0x08: Rising trigger selection */
  __IO uint32_t FTSR;  /**< 0x0C: Falling trigger selection */
  __IO uint32_t SWIER; /**< 0x10: Software interrupt event */
  __IO uint32_t PR;    /**< 0x14: Pending (write 1 to clear) */
} EXTI_Typedef;

#define EXTI_BASE_ADDR 0x40013C00UL
#define EXTI ((EXTI_Typedef *)EXTI_BASE_ADDR)

/* Internal lines used by the RTC. */
#define EXTI_LINE_RTC_ALARM (1U << 17)
#define EXTI_LINE_RTC_WAKEUP (1U << 22)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M4_EXTI_REG_H */
