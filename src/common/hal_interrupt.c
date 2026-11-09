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
 * @file common/hal_interrupt.c
 * @brief Shared public interrupt layer: validate, then dispatch to the backend.
 *
 * @details
 * The NULL-callback check lives here so all four backends agree on it. IRQ
 * bounds stay with the backend: the valid range is an arch property, and a
 * shared layer that guessed it would be wrong on three ports out of four.
 */

#include "common/hal_interrupt.h"
#include "internal/hal_interrupt_ops.h"

#if NAVHAL_CONFIG_DRV_INTERRUPT

#include "internal/hal_interrupt_table.h"
#include <stdbool.h>
#include <stddef.h>

hal_status_t hal_interrupt_enable(hal_irq_t irq) {
  return _hal_interrupt_ops.enable(irq);
}

hal_status_t hal_interrupt_disable(hal_irq_t irq) {
  return _hal_interrupt_ops.disable(irq);
}

/* The callback table. Every port had this: an array, a bounds check and an
 * indexed call, written out three times over three private arrays. It is the
 * same code each time because nothing about it is hardware -- what differs is
 * only how many lines the part wires, which the family header states. */
static hal_interrupt_callback_t _irq_cb[HAL_IRQ_COUNT];

/* Negative hal_irq_t values are system exceptions, which have their own
 * vectors and never reach the table. */
static inline bool _irq_indexes_table(hal_irq_t irq) {
  return (long)irq >= 0 && (unsigned long)irq < (unsigned long)HAL_IRQ_COUNT;
}

bool navhal_irq_invoke(hal_irq_t irq) {
  if (!_irq_indexes_table(irq) || _irq_cb[(unsigned long)irq] == NULL)
    return false;
  _irq_cb[(unsigned long)irq]();
  return true;
}

hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t callback) {
  if (callback == NULL || !_irq_indexes_table(irq))
    return HAL_ERR_INVALID_ARG;
  _irq_cb[(unsigned long)irq] = callback;
  return HAL_OK;
}

hal_status_t hal_interrupt_detach_callback(hal_irq_t irq) {
  if (!_irq_indexes_table(irq))
    return HAL_ERR_INVALID_ARG;
  _irq_cb[(unsigned long)irq] = NULL;
  return HAL_OK;
}

void hal_interrupt_dispatch(hal_irq_t irq) {
  _hal_interrupt_ops.dispatch(irq);
}

/* Derived, not a port op: priority first, because enabling first leaves a
 * window in which the line can fire at the reset-default priority 0. A port
 * without programmable priorities answers HAL_ERR_NOT_SUPPORTED to the first
 * call, which is not a reason to skip the second. */
hal_status_t hal_interrupt_enable_with_priority(hal_irq_t irq,
                                                uint8_t priority) {
  hal_status_t st = _hal_interrupt_ops.set_priority(irq, priority);
  if (st != HAL_OK && st != HAL_ERR_NOT_SUPPORTED)
    return st;
  return _hal_interrupt_ops.enable(irq);
}

void hal_cpu_idle(void) { _hal_interrupt_ops.cpu_idle(); }

uint32_t hal_interrupt_disable_global(void) {
  return _hal_interrupt_ops.disable_global();
}

void hal_interrupt_enable_global(uint32_t state) {
  _hal_interrupt_ops.enable_global(state);
}

hal_status_t hal_interrupt_set_priority(hal_irq_t irq, uint8_t priority) {
  return _hal_interrupt_ops.set_priority(irq, priority);
}

uint8_t hal_interrupt_get_priority(hal_irq_t irq) {
  return _hal_interrupt_ops.get_priority(irq);
}

bool hal_interrupt_is_pending(hal_irq_t irq) {
  return _hal_interrupt_ops.is_pending(irq);
}

hal_status_t hal_interrupt_clear_pending(hal_irq_t irq) {
  return _hal_interrupt_ops.clear_pending(irq);
}

void hal_interrupt_clear_all_pending(void) {
  _hal_interrupt_ops.clear_all_pending();
}

#endif /* NAVHAL_CONFIG_DRV_INTERRUPT */
