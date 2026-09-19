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
 * @file reset.c
 * @brief STM32 implementation of hal_reset.h.
 *
 * @details
 * The cause flags live in RCC_CSR and the reset request lives in the Cortex-M
 * SCB, so this file spans both — the API is one question and splitting it
 * across two drivers would only make callers assemble the answer themselves.
 */

/* The TEST build globs every vendor source regardless of Kconfig, so the gate
 * has to be in the file as well as in CMake — otherwise the driver links into
 * builds that never asked for it. */
#if NAVHAL_CONFIG_DRV_RESET

#include "common/hal_reset.h"
#include "internal/hal_reset_ops.h"
#include "family/rcc_reg.h"

/* SCB_AIRCR. There is no SCB definition in the port headers, and one register
 * does not earn a whole map. */
#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0CUL)
#define SCB_AIRCR_VECTKEY 0x05FA0000U /**< Writes without this key are ignored. */
#define SCB_AIRCR_SYSRESETREQ (1U << 2)

static uint32_t latched_cause;
static uint8_t cause_valid;

static hal_status_t stm32_reset_init(void) {
  /* Latch once. A second call must not overwrite the real cause with the
   * zeroes left behind by the first one's RMVF. */
  if (cause_valid)
    return HAL_OK;

  const uint32_t csr = RCC->CSR;
  uint32_t cause = HAL_RESET_CAUSE_UNKNOWN;

  if (csr & RCC_CSR_PORRSTF)
    cause |= HAL_RESET_CAUSE_POWER_ON;
  if (csr & RCC_CSR_PINRSTF)
    cause |= HAL_RESET_CAUSE_PIN;
  if (csr & RCC_CSR_BORRSTF)
    cause |= HAL_RESET_CAUSE_BROWNOUT;
  if (csr & RCC_CSR_SFTRSTF)
    cause |= HAL_RESET_CAUSE_SOFTWARE;
  if (csr & RCC_CSR_IWDGRSTF)
    cause |= HAL_RESET_CAUSE_WATCHDOG;
  if (csr & RCC_CSR_WWDGRSTF)
    cause |= HAL_RESET_CAUSE_WINDOW_WATCHDOG;
  if (csr & RCC_CSR_LPWRRSTF)
    cause |= HAL_RESET_CAUSE_LOW_POWER;

  latched_cause = cause;
  cause_valid = 1;

  /* Clear, or the next boot reports this one's cause as well as its own. RMVF
   * is write-1-to-clear and the other CSR bits (LSION/LSIRDY) are not, so a
   * read-modify-write is what keeps the LSI alone. */
  RCC->CSR |= RCC_CSR_RMVF;
  RCC->CSR &= ~RCC_CSR_RMVF;

  return HAL_OK;
}

static uint32_t stm32_reset_get_cause(void) { return latched_cause; }

static hal_status_t stm32_reset_system_reset(void) {
  /* Drain the write buffer first: the reset can otherwise land before an
   * earlier store to a peripheral has left the core. */
  __asm volatile("dsb 0xF" ::: "memory");
  SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
  __asm volatile("dsb 0xF" ::: "memory");
  for (;;) {
    /* The reset is not instantaneous. Spinning here keeps the caller from
     * running on into code that assumes it never returned. */
  }

  return HAL_OK; /* unreachable; the op returns hal_status_t. */
}


/** @brief The STM32 reset backend. */
const hal_reset_ops_t _hal_reset_ops = {
    .init = stm32_reset_init,
    .get_cause = stm32_reset_get_cause,
    .system_reset = stm32_reset_system_reset,
};

#endif /* NAVHAL_CONFIG_DRV_RESET */
