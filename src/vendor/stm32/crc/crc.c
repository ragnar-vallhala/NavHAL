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
 * When @c _CRC_HW_ENABLED is defined this drives the STM32F4 hardware CRC unit.
 * Otherwise it adopts the portable software CRC-32/MPEG-2 shared by the common
 * layer (@c hal_crc_sw_* ) — there is no STM32-specific software CRC code, it
 * is the same algorithm every software-CRC port uses. Argument validation
 * (NULL cfg) lives in the shared public layer src/common/hal_crc.c.
 */

#include "internal/hal_crc_ops.h"

#ifdef _CRC_HW_ENABLED

#include "family/crc_reg.h"
#include "family/rcc_reg.h"

/* Configured init value (HW path only). */
static uint32_t s_crc_init_value = 0xFFFFFFFF;

static hal_status_t stm32_crc_reset(void) {
  CRC->CR = CRC_CR_RESET;
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

static uint32_t stm32_crc_accumulate(const uint8_t *data, uint32_t len) {
  if (data == NULL || len == 0) {
    return CRC->DR;
  }

  uint32_t i = 0;

  /* The STM32F4 CRC unit operates on 32-bit words.
     When feeding bytes, we pack 4 bytes into a word (Big Endian order
     to match standard CRC32 expectations when fed this way). */
  while ((len - i) >= 4) {
    uint32_t word = ((uint32_t)data[i] << 24) | ((uint32_t)data[i + 1] << 16) |
                    ((uint32_t)data[i + 2] << 8) | ((uint32_t)data[i + 3]);
    CRC->DR = word;
    i += 4;
  }

  /* Handle remaining 1-3 bytes */
  if (i < len) {
    /* On standard STM32F4, we can't do byte-wise writes easily
       without messing up the polynomial shifting. Standard practice
       for arbitrary length buffers on this specific hardware is
       often tricky. To be perfectly compatible with byte-stream
       software CRC, we must pad and do manual XORs, but typically
       hardware CRC is used on word-aligned block multiples.
       For this HAL, we pack the remaining bytes into a word. */
    uint32_t word = 0;

    if ((len - i) >= 1)
      word |= ((uint32_t)data[i++] << 24);
    if ((len - i) >= 1)
      word |= ((uint32_t)data[i++] << 16);
    if ((len - i) >= 1)
      word |= ((uint32_t)data[i++] << 8);

    CRC->DR = word;
  }

  return CRC->DR;
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

#endif /* _CRC_HW_ENABLED */
