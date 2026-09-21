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
 * @file internal/hal_interrupt_ops.h
 * @brief HAL-internal interrupt-controller backend interface.
 *
 * @details
 * Four backends implement this today -- ARMv7E-M NVIC, AVR, and the PC 8259 --
 * which is why it earns a table at all.
 *
 * The surface is the one ARM and AVR already share. Two things stay out:
 *
 *  - @c hal_interrupt_enable_with_priority is Cortex-M only and remains a
 *    port extension declared in that port's header, the same way the UART
 *    idle callback is.
 *  - IRQ bounds checking stays in each backend, because the valid range is an
 *    arch property the shared layer cannot know. What hoists is the
 *    NULL-callback check, which every backend repeated.
 *
 * Where a controller genuinely cannot do something -- 8259 priority is fixed
 * by line number, not programmable -- the backend reports
 * ::HAL_ERR_NOT_SUPPORTED rather than the entry being left NULL, so the
 * build-time completeness check still means something.
 */

#ifndef NAVHAL_INTERNAL_HAL_INTERRUPT_OPS_H
#define NAVHAL_INTERNAL_HAL_INTERRUPT_OPS_H

#include "common/hal_interrupt.h"
#include "common/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_INTERRUPT

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port interrupt-controller operations table. */
typedef struct {
  /** Unmask one line. */
  hal_status_t (*enable)(hal_irq_t irq);
  /** Mask one line. */
  hal_status_t (*disable)(hal_irq_t irq);

  /** Install a callback. NULL @p cb rejected upstream. */
  hal_status_t (*attach_callback)(hal_irq_t irq, hal_interrupt_callback_t cb);
  /** Remove a callback. */
  hal_status_t (*detach_callback)(hal_irq_t irq);
  /** Run the callback registered for @p irq, from the vector. */
  void (*dispatch)(hal_irq_t irq);

  /** Mask interrupts globally; returns the previous state to restore. */
  uint32_t (*disable_global)(void);
  /** Restore a state returned by ::disable_global. */
  void (*enable_global)(uint32_t state);

  /** Programmable priority, or ::HAL_ERR_NOT_SUPPORTED. */
  hal_status_t (*set_priority)(hal_irq_t irq, uint8_t priority);
  /** Current priority, 0 where priority is not programmable. */
  uint8_t (*get_priority)(hal_irq_t irq);

  /** Is @p irq pending? */
  bool (*is_pending)(hal_irq_t irq);
  /** Clear one pending line. */
  hal_status_t (*clear_pending)(hal_irq_t irq);
  /** Clear every pending line. */
  void (*clear_all_pending)(void);
} hal_interrupt_ops_t;

/** @brief The active port's interrupt backend. */
extern const hal_interrupt_ops_t _hal_interrupt_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_CONFIG_DRV_INTERRUPT */

#endif /* NAVHAL_INTERNAL_HAL_INTERRUPT_OPS_H */
