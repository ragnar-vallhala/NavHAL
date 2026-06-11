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
 * @file fpu.c
 * @brief Standardized HAL hardware-FPU driver for Cortex-M4.
 */

#include "navhal_port_fpu.h"
#include <stdint.h>

#define CPACR (*(volatile uint32_t *)0xE000ED88)
#define FPCCR (*(volatile uint32_t *)0xE000EF34)

#ifdef _FPU_ENABLED
hal_status_t hal_fpu_enable(void) {
  // CPACR: Enable full access to CP10 and CP11
  CPACR |= (0xF << 20);

  // FPCCR: Enable lazy stacking (ASPEN and LSPEN bits)
  FPCCR |= (0x3 << 30);

  // Ensure all pipeline operations are complete
  __asm volatile("dsb");
  __asm volatile("isb");
  return HAL_OK;
}
#endif
