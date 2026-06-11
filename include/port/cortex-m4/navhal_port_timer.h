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
 * @file port/cortex-m4/navhal_port_timer.h
 * @brief Cortex-M4 / STM32F4 timer + SysTick port header.
 *
 * @details
 * The public timer/timebase API lives in @c common/hal_timer.h, which
 * includes this header. This file carries the SysTick and RCC register-bit
 * defines plus the vector-table ISR entries (@c SysTick_Handler,
 * @c TIMx_IRQHandler).
 */

#ifndef NAVHAL_PORT_TIMER_H
#define NAVHAL_PORT_TIMER_H

#include "common/hal_timer.h"
#include "common/hal_types.h"


#ifdef __cplusplus
extern "C" {
#endif

// APB1
#define RCC_APB1ENR_TIM2_OFFSET 0
#define RCC_APB1ENR_TIM3_OFFSET 1
#define RCC_APB1ENR_TIM4_OFFSET 2
#define RCC_APB1ENR_TIM5_OFFSET 3
// APB2
#define RCC_APB2ENR_TIM1_OFFSET 0
#define RCC_APB2ENR_TIM9_OFFSET 16
#define RCC_APB2ENR_TIM10_OFFSET 17
#define RCC_APB2ENR_TIM11_OFFSET 18

// SysTick Control and Status Register
#define SYST_CSR (*(__IO uint32_t *)0xE000E010)
// SysTick Reload Value Register
#define SYST_RVR (*(__IO uint32_t *)0xE000E014)
// SysTick Current Value Register
#define SYST_CVR (*(__IO uint32_t *)0xE000E018)
// SysTick Calibration Register
#define SYST_CALIB (*(__IO uint32_t *)0xE000E01C)
#define SYST_CSR_EN_BIT 0
#define SYST_CSR_TICKINT_BIT 1
#define SYST_CSR_CLKSOURCE_BIT 2

/** @brief SysTick exception handler (vector-table entry). */
void SysTick_Handler(void);

// Timer IRQ Handlers
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);
void TIM9_IRQHandler(void);
void TIM12_IRQHandler(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization timebase/timer names — retained as a backward-compat alias. */
#include "compat/timebase_compat.h"
#include "compat/timer_compat.h"

#endif /* NAVHAL_PORT_TIMER_H */
