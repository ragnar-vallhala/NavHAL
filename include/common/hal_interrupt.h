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
#if NAVHAL_CONFIG_DRV_INTERRUPT

#include <stdbool.h>
#include <stdint.h>

#include "navhal_port_interrupt.h"

/**
 * @brief Callback invoked by ::hal_interrupt_dispatch for a registered IRQ.
 */
typedef void (*hal_interrupt_callback_t)(void);

/**
 * @brief Enable a specific interrupt in the NVIC.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_enable(hal_irq_t irq);

/**
 * @brief Default NVIC priority level applied by ::hal_interrupt_enable.
 *
 * A mid-range level (range 0-15) rather than the NVIC reset default of 0.
 * Priority 0 is the most urgent and is NOT maskable by an RTOS `BASEPRI`
 * critical section, so an IRQ left at 0 whose handler calls a `*_from_isr`
 * kernel API can preempt and corrupt the scheduler. This default keeps enabled
 * lines maskable by typical RTOS syscall thresholds. Use
 * ::hal_interrupt_enable_with_priority or ::hal_interrupt_set_priority for
 * explicit control.
 */
#define HAL_IRQ_PRIORITY_DEFAULT 8u


/**
 * @brief Disable a specific interrupt in the NVIC.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_disable(hal_irq_t irq);

/**
 * @brief Clear the pending flag of a specific interrupt.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_clear_pending(hal_irq_t irq);

/**
 * @brief Set an interrupt's priority level.
 *
 * @param irq      The line.
 * @param priority A LEVEL, 0 (most urgent) to the implemented maximum -- 15 on
 *                 these Cortex-M parts. Not a raw register value: passing 0xE0
 *                 to mean level 14 is ::HAL_ERR_INVALID_ARG, not level 0.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for a line that does not exist or a
 *         level that does not fit, ::HAL_ERR_NOT_SUPPORTED for an exception
 *         whose priority the architecture fixes.
 */
hal_status_t hal_interrupt_set_priority(hal_irq_t irq, uint8_t priority);

/**
 * @brief Get the priority of a specific interrupt or system exception.
 * @param irq IRQ number.
 * @return Normalized priority value.
 */
uint8_t hal_interrupt_get_priority(hal_irq_t irq);

/**
 * @brief Check whether a specific interrupt is pending.
 * @param irq IRQ number.
 * @return true if pending, false otherwise.
 */
bool hal_interrupt_is_pending(hal_irq_t irq);

/**
 * @brief Register a callback to be invoked for a specific IRQ.
 * @param irq      IRQ number.
 * @param callback Callback function, or NULL to clear.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t callback);

/**
 * @brief Remove the callback registered for a specific IRQ.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_detach_callback(hal_irq_t irq);

/**
 * @brief Invoke the callback registered for an IRQ (called from an ISR).
 * @param irq IRQ number that occurred.
 */
void hal_interrupt_dispatch(hal_irq_t irq);

/**
 * @brief Restore the global interrupt-enable state (PRIMASK).
 * @param state State previously returned by ::hal_interrupt_disable_global.
 */
void hal_interrupt_enable_global(uint32_t state);

/**
 * @brief Disable all maskable interrupts globally.
 * @return The previous global interrupt state, for use with
 *         ::hal_interrupt_enable_global.
 */
uint32_t hal_interrupt_disable_global(void);

/**
 * @brief Leave nothing pending.
 *
 * The result is the contract -- after this call, no line is pending -- and how
 * a port reaches it is its own business. An NVIC clears the lot with three
 * ICPR writes; a part whose flags live in their peripherals walks them. A
 * portable loop in this layer would have cost the NVIC up to 82 iterations for
 * nothing, which is why this is a port op rather than a derived one.
 *
 * Flags that cannot be cleared without a side effect are left alone, and
 * ::hal_interrupt_clear_pending names them one at a time: the AVR's TWINT
 * releases the TWI bus when written, and the USART and SPI flags clear by
 * reading their data registers.
 */
void hal_interrupt_clear_all_pending(void);

/**
 * @brief Set a line's priority, then enable it.
 *
 * Priority first: enabling first leaves a window in which the line can fire at
 * the reset-default priority 0. A port without programmable priorities answers
 * ::HAL_ERR_NOT_SUPPORTED to the priority half and is still enabled.
 */
hal_status_t hal_interrupt_enable_with_priority(hal_irq_t irq, uint8_t priority);

/**
 * @brief Sleep until an interrupt would occur.
 *
 * Returns with the caller's global interrupt state as it was. Where the sleep
 * instruction needs interrupts enabled -- AVR's SLEEP, x86's HLT, neither of
 * which wakes while masked -- the port enables them across the instruction and
 * puts the state back, so a handler may run before this returns. Cortex-M's
 * WFI wakes regardless of PRIMASK, so there a masked wake stays pending.
 *
 * A wake condition already pending on entry returns immediately, so this
 * cannot deadlock.
 */
void hal_cpu_idle(void);
 // architecture-specific interrupt definitions
/* The deprecated-name shims. They live here rather than in the port header
 * because the shim includes that header for the declarations it wraps, and
 * the two including each other is a cycle -- which compiles, thanks to the
 * include guards, and which clang-tidy's misc-header-include-cycle reports
 * at every call site that opens one of them. */
#include "compat/interrupt_compat.h"
/* Inside the gate with the port header: a vendor that does not implement the
 * interrupt driver has no reason to ship an interrupt register map, and this
 * include used to demand one from every port regardless. */
#include "family/interrupt_reg.h" // vendor interrupt register definitions
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_INTERRUPT */
#endif // !HAL_INTERRUPT_H