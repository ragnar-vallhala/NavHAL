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

#ifndef HAL_INTERRUPT_H
#define HAL_INTERRUPT_H

/**
 * @defgroup HAL_INTERRUPT Interrupt
 * @ingroup HAL_DRIVERS
 * @brief Interrupt enable, priority, and callback registration.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file hal_interrupt.h
 * @brief Common Interrupt HAL interface for NavHAL.
 *
 * This header defines a common interface for managing hardware interrupts
 * across different microcontroller architectures. It provides functions to
 * enable/disable interrupts, attach/detach callbacks, and set interrupt
 * priorities in an architecture-agnostic manner.
 *
 * Supported architectures include Cortex-M4 (STM32F4 series).
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */
#include "navhal_port_interrupt.h" // Include architecture-specific interrupt definitions
#include "family/interrupt_reg.h" // Include architecture-specific interrupt register definitions

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_INTERRUPT */
#endif // !HAL_INTERRUPT_H