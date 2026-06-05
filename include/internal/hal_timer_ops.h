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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-vendor timer operations table. */
typedef struct {
  hal_status_t (*init)(hal_timer_t timer, const hal_timer_config_t *cfg);
  hal_status_t (*init_freq)(hal_timer_t timer, uint32_t freq);
  hal_status_t (*start)(hal_timer_t timer);
  hal_status_t (*stop)(hal_timer_t timer);
  hal_status_t (*reset)(hal_timer_t timer);
  uint32_t (*get_count)(hal_timer_t timer);
  hal_status_t (*enable_interrupt)(hal_timer_t timer);
  hal_status_t (*disable_interrupt)(hal_timer_t timer);
  hal_status_t (*clear_interrupt_flag)(hal_timer_t timer);
  hal_status_t (*attach_callback)(hal_timer_t timer,
                                  hal_timer_callback_t callback);
  hal_status_t (*detach_callback)(hal_timer_t timer);
  hal_status_t (*set_compare)(hal_timer_t timer, uint8_t channel,
                              uint32_t compare_value);
  uint32_t (*get_compare)(hal_timer_t timer, uint32_t channel);
  hal_status_t (*enable_channel)(hal_timer_t timer, uint32_t channel);
  hal_status_t (*disable_channel)(hal_timer_t timer, uint32_t channel);
  uint32_t (*get_frequency)(hal_timer_t timer);
  hal_status_t (*set_prescaler)(hal_timer_t timer, uint32_t prescaler);
  hal_status_t (*set_auto_reload)(hal_timer_t timer, uint32_t auto_reload);
  uint32_t (*get_auto_reload)(hal_timer_t timer);
} hal_timer_ops_t;

/** @brief The active port's timer operations table (defined by one backend). */
extern const hal_timer_ops_t _hal_timer_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_TIMER_OPS_H */
