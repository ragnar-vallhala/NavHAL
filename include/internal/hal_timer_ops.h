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
 * @file internal/hal_timer_ops.h
 * @brief HAL-internal timer vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_timer.h.
 * The timer register model is wholly vendor-specific (STM32 TIMx vs ATmega TCx),
 * so this is a 1:1 dispatch table: one entry per public call, dedup limited to
 * the shared NULL-config validation in src/common/hal_timer.c. See
 * @c internal/hal_gpio_ops.h for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_TIMER_OPS_H
#define NAVHAL_INTERNAL_HAL_TIMER_OPS_H

#include "common/hal_timer.h"
#include "common/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-vendor timer operations table. */
typedef struct {
  /* ---- configuration ------------------------------------------------- */
  /** Backend for ::hal_timer_init. Validation (NULL @p cfg) has already run. */
  hal_status_t (*init)(hal_timer_t timer, const hal_timer_config_t *cfg);
  /**
   * Program a time base of @p ticks input-clock periods.
   *
   * This is the one part of frequency setup that cannot be shared: the
   * achievable (divider, reload) pairs are a hardware property. STM32 solves
   * an arbitrary PSC/ARR; the AVR searches a fixed divider table
   * (1/8/64/256/1024). The shared layer does the portable half -- reject
   * freq == 0, read the input clock, divide -- and hands the result here.
   */
  hal_status_t (*set_timebase)(hal_timer_t timer, uint64_t ticks);
  /** Frequency in Hz feeding this timer's prescaler. */
  uint32_t (*get_input_clock)(hal_timer_t timer);

  /* ---- run state ------------------------------------------------------ */
  /** Start (@p on true) or stop the counter. */
  hal_status_t (*set_running)(hal_timer_t timer, bool on);
  /** Backend for ::hal_timer_reset -- zero the counter. */
  hal_status_t (*reset)(hal_timer_t timer);
  /** Backend for ::hal_timer_get_count. */
  uint32_t (*get_count)(hal_timer_t timer);

  /* ---- time base registers -------------------------------------------- */
  /**
   * Backend for ::hal_timer_set_prescaler. NOTE: v1 documents this argument
   * as the STM32 PSC register value, and the AVR backend instead snaps it to
   * the nearest achievable divider. That divergence is pre-existing and is
   * deliberately preserved here; ::get_divider exists so the shared layer
   * never has to guess which meaning applies.
   */
  hal_status_t (*set_prescaler)(hal_timer_t timer, uint32_t prescaler);
  /** Effective divider currently applied (not a register value). */
  uint32_t (*get_divider)(hal_timer_t timer);
  /** Backend for ::hal_timer_set_auto_reload. */
  hal_status_t (*set_auto_reload)(hal_timer_t timer, uint32_t auto_reload);
  /** Backend for ::hal_timer_get_auto_reload. */
  uint32_t (*get_auto_reload)(hal_timer_t timer);

  /* ---- interrupts ------------------------------------------------------ */
  /** Enable (@p on true) or disable the update interrupt. */
  hal_status_t (*set_interrupt)(hal_timer_t timer, bool on);
  /** Backend for ::hal_timer_clear_interrupt_flag. */
  hal_status_t (*clear_interrupt_flag)(hal_timer_t timer);
  /** Install @p callback, or detach when it is NULL. */
  hal_status_t (*set_callback)(hal_timer_t timer,
                               hal_timer_callback_t callback);

  /* ---- capture / compare ------------------------------------------------ */
  /** Enable (@p on true) or disable an output channel. */
  hal_status_t (*set_channel_enabled)(hal_timer_t timer, uint32_t channel,
                                      bool on);
  /** Backend for ::hal_timer_set_compare. */
  hal_status_t (*set_compare)(hal_timer_t timer, uint8_t channel,
                              uint32_t compare_value);
  /** Backend for ::hal_timer_get_compare. */
  uint32_t (*get_compare)(hal_timer_t timer, uint32_t channel);
} hal_timer_ops_t;

/** @brief The active port's timer operations table (defined by one backend). */
extern const hal_timer_ops_t _hal_timer_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_TIMER_OPS_H */
