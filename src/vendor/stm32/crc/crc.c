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
 * @file crc.c
 * @brief CRC vendor backend for STM32F4 (the driver vtable).
 *
 * @details
 * When @c NAVHAL_CONFIG_DRV_CRC is defined this drives the STM32F4 hardware CRC unit.
 * Otherwise it adopts the portable software CRC-32/MPEG-2 shared by the common
 * layer (@c hal_crc_sw_* ) — there is no STM32-specific software CRC code, it
 * is the same algorithm every software-CRC port uses. Argument validation
 * (NULL cfg) lives in the shared public layer src/common/hal_crc.c.
 */

#include "internal/hal_crc_ops.h"

#include <stdbool.h>

#if NAVHAL_CONFIG_DRV_CRC

#include "family/crc_reg.h"
#include "family/rcc_reg.h"

/* Configured init value (HW path only). */
static uint32_t s_crc_init_value = 0xFFFFFFFF;

static uint32_t s_current = 0xFFFFFFFFu;
static bool s_hw_in_step = true;

static hal_status_t stm32_crc_reset(void) {
  CRC->CR = CRC_CR_RESET;
  s_current = 0xFFFFFFFFu;
  s_hw_in_step = true;
  /* Hardware always resets to 0xFFFFFFFF. If a different init value
     was requested, we would ideally write it here, but STM32F4 CRC
     doesn't let us write a custom init value without extra XOR math
     (or it does on newer F4s via INIT register, but standard F401/411
     doesn't have it). For our API, we only guarantee 0xFFFFFFFF works
     natively on all F4 hardware for now. */
  return HAL_OK;
}

static hal_status_t stm32_crc_init(const hal_crc_config_t *cfg) {
  /* cfg is non-NULL: the public layer validated it before dispatching. */
  s_crc_init_value = cfg->init_value;

  /* Enable CRC clock */
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

  /* Issue reset */
  stm32_crc_reset();
  return HAL_OK;
}

/* The running value, and whether the peripheral still holds it.
 *
 * The unit only consumes whole words. A byte write to CRC_DR does not feed one
 * byte -- it feeds a padded word, which is measurable: every length that is a
 * multiple of four agrees with the reference implementation and every other
 * length does not. And this part has no INIT register, so once the true value
 * and the peripheral's register disagree, the peripheral cannot be put back in
 * step.
 *
 * So the peripheral takes the whole words, software finishes any tail with the
 * shared table, and from the first tail onward the running value lives here. The
 * fast path is the common one: a stream of word-multiple chunks never leaves the
 * hardware. What this buys is the contract hal_crc.h actually promises --
 * byte-exact, composable across calls, and the same answer as every other
 * implementation of this CRC.
 *
 * It used to pack a tail into a word and zero-pad it, feeding up to three bytes
 * that were never in the buffer. The result matched nothing else, including this
 * driver's own software fallback, and the API said otherwise. It cost a
 * debugging session in the loader's recovery protocol, which went to a software
 * CRC instead.
 */

static uint32_t stm32_crc_accumulate(const uint8_t *data, uint32_t len) {
  if (data == NULL || len == 0)
    return s_current;

  uint32_t i = 0;

  if (s_hw_in_step) {
    /* Four bytes written big-endian as one word give the same result as the same
       four bytes fed individually, which is why the bulk can go in as words. */
    while ((len - i) >= 4u) {
      CRC->DR = ((uint32_t)data[i] << 24) | ((uint32_t)data[i + 1] << 16) |
                ((uint32_t)data[i + 2] << 8) | ((uint32_t)data[i + 3]);
      i += 4u;
    }
    s_current = CRC->DR;
  }

  if (i < len) {
    s_current = hal_crc_sw_step(s_current, &data[i], len - i);
    s_hw_in_step = false; /* the register is behind now, and cannot be reseeded */
  }

  return s_current;
}

static uint32_t stm32_crc_compute(const uint8_t *data, uint32_t len) {
  stm32_crc_reset();
  return stm32_crc_accumulate(data, len);
}

const hal_crc_ops_t _hal_crc_ops = {
    .init = stm32_crc_init,
    .compute = stm32_crc_compute,
    .accumulate = stm32_crc_accumulate,
    .reset = stm32_crc_reset,
};

#else /* software path — adopt the shared CRC-32/MPEG-2 implementation. */

const hal_crc_ops_t _hal_crc_ops = {
    .init = hal_crc_sw_init,
    .compute = hal_crc_sw_compute,
    .accumulate = hal_crc_sw_accumulate,
    .reset = hal_crc_sw_reset,
};

#endif /* NAVHAL_CONFIG_DRV_CRC */
