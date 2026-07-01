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
 * @file host_stubs.c
 * @brief Minimal host stubs for the arch-layer functions the vendor drivers
 *        call but that aren't part of the driver suite (NVIC, timebase, FPU).
 *
 * The timebase counter advances on every read so that driver timeout loops
 * (`hal_spi_*`) terminate deterministically.
 */

#include "common/hal_status.h"
#include "navhal_port_interrupt.h"
#include <stdint.h>

static uint32_t s_millis = 0;
uint32_t hal_timebase_get_millis(void) { return s_millis++; }
uint32_t hal_timebase_get_micros(void) { return s_millis++; }
uint32_t hal_timebase_get_tick(void) { return s_millis++; }

hal_status_t hal_interrupt_enable(hal_irq_t irq) {
  (void)irq;
  return HAL_OK;
}
hal_status_t hal_interrupt_disable(hal_irq_t irq) {
  (void)irq;
  return HAL_OK;
}
