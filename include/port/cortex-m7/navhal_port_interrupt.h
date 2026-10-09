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
 * @file port/cortex-m4/navhal_port_interrupt.h
 * @brief Cortex-M4 / STM32F4 interrupt-controller (NVIC) HAL driver interface.
 *
 * @details
 * Standardized interrupt API (see `docs/api_standardization.md`). All public
 * functions use the `hal_interrupt_` prefix and `snake_case` verbs. Per-IRQ
 * control operations return ::hal_status_t; queries return their value
 * directly. IRQs are identified by @c hal_irq_t.
 */

#ifndef NAVHAL_PORT_INTERRUPT_H
#define NAVHAL_PORT_INTERRUPT_H

#include "common/hal_status.h"
#include "family/interrupt_reg.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Callback invoked by ::hal_interrupt_dispatch for a registered IRQ.
 */
typedef void (*hal_interrupt_callback_t)(void);















/* Deprecated pre-standardization interrupt names — retained as a backward-compat alias. */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_PORT_INTERRUPT_H
