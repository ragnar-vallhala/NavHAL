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
 * @file src/common/hal_timer.c
 * @brief Shared public timer layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The timer register model is entirely vendor-specific, so this is a 1:1
 * dispatch layer. The one piece of shared contract it enforces is the
 * NULL-config check on ::hal_timer_init; everything else forwards to the active
 * backend's ::_hal_timer_ops.
 */

#include "common/hal_timer.h"
#include "internal/hal_timer_ops.h"

#include <stddef.h>

hal_status_t hal_timer_init(hal_timer_t timer, const hal_timer_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_timer_ops.init(timer, cfg);
}

hal_status_t hal_timer_init_freq(hal_timer_t timer, uint32_t freq) {
  return _hal_timer_ops.init_freq(timer, freq);
}

hal_status_t hal_timer_start(hal_timer_t timer) {
  return _hal_timer_ops.start(timer);
}

hal_status_t hal_timer_stop(hal_timer_t timer) {
  return _hal_timer_ops.stop(timer);
}

hal_status_t hal_timer_reset(hal_timer_t timer) {
  return _hal_timer_ops.reset(timer);
}

uint32_t hal_timer_get_count(hal_timer_t timer) {
  return _hal_timer_ops.get_count(timer);
}

hal_status_t hal_timer_enable_interrupt(hal_timer_t timer) {
  return _hal_timer_ops.enable_interrupt(timer);
}

hal_status_t hal_timer_disable_interrupt(hal_timer_t timer) {
  return _hal_timer_ops.disable_interrupt(timer);
}

hal_status_t hal_timer_clear_interrupt_flag(hal_timer_t timer) {
  return _hal_timer_ops.clear_interrupt_flag(timer);
}

hal_status_t hal_timer_attach_callback(hal_timer_t timer,
                                       hal_timer_callback_t callback) {
  return _hal_timer_ops.attach_callback(timer, callback);
}

hal_status_t hal_timer_detach_callback(hal_timer_t timer) {
  return _hal_timer_ops.detach_callback(timer);
}

hal_status_t hal_timer_set_compare(hal_timer_t timer, uint8_t channel,
                                   uint32_t compare_value) {
  return _hal_timer_ops.set_compare(timer, channel, compare_value);
}

uint32_t hal_timer_get_compare(hal_timer_t timer, uint32_t channel) {
  return _hal_timer_ops.get_compare(timer, channel);
}

hal_status_t hal_timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  return _hal_timer_ops.enable_channel(timer, channel);
}

hal_status_t hal_timer_disable_channel(hal_timer_t timer, uint32_t channel) {
  return _hal_timer_ops.disable_channel(timer, channel);
}

uint32_t hal_timer_get_frequency(hal_timer_t timer) {
  return _hal_timer_ops.get_frequency(timer);
}

hal_status_t hal_timer_set_prescaler(hal_timer_t timer, uint32_t prescaler) {
  return _hal_timer_ops.set_prescaler(timer, prescaler);
}

hal_status_t hal_timer_set_auto_reload(hal_timer_t timer, uint32_t auto_reload) {
  return _hal_timer_ops.set_auto_reload(timer, auto_reload);
}

uint32_t hal_timer_get_auto_reload(hal_timer_t timer) {
  return _hal_timer_ops.get_auto_reload(timer);
}
