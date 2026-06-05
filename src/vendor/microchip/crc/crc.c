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
 * @file src/vendor/microchip/crc/crc.c
 * @brief ATmega328P CRC vendor backend — software CRC-32/MPEG-2.
 *
 * @details
 * Provides the ATmega328P implementations behind ::hal_crc_ops_t. The part has
 * no hardware CRC unit, so this is a bitwise software CRC-32/MPEG-2 (polynomial
 * 0x04C11DB7, non-reflected, no final XOR) — matching the STM32F4 hardware
 * result. A bitwise loop is used rather than a 1 KiB lookup table to keep the
 * flash footprint small. Argument validation (NULL cfg) lives in the shared
 * public layer src/common/hal_crc.c; the table is published as ::_hal_crc_ops.
 */

#include "internal/hal_crc_ops.h"

#include <stddef.h>

/** @brief Configured init value, applied by the reset op. */
static uint32_t s_init = 0xFFFFFFFFu;
/** @brief Running accumulator. */
static uint32_t s_acc = 0xFFFFFFFFu;

/** @brief Fold @p len bytes into @p crc — CRC-32/MPEG-2, MSB-first. */
static uint32_t crc32_mpeg2(uint32_t crc, const uint8_t *data, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    crc ^= (uint32_t)data[i] << 24;
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80000000UL)
        crc = (crc << 1) ^ 0x04C11DB7UL;
      else
        crc <<= 1;
    }
  }
  return crc;
}

static hal_status_t avr_crc_init(const hal_crc_config_t *cfg) {
  /* cfg is non-NULL: the public layer validated it before dispatching. */
  s_init = cfg->init_value;
  s_acc = s_init;
  return HAL_OK;
}

static hal_status_t avr_crc_reset(void) {
  s_acc = s_init;
  return HAL_OK;
}

static uint32_t avr_crc_accumulate(const uint8_t *data, uint32_t len) {
  if (data != NULL)
    s_acc = crc32_mpeg2(s_acc, data, len);
  return s_acc;
}

static uint32_t avr_crc_compute(const uint8_t *data, uint32_t len) {
  s_acc = s_init;
  return avr_crc_accumulate(data, len);
}

const hal_crc_ops_t _hal_crc_ops = {
    .init = avr_crc_init,
    .compute = avr_crc_compute,
    .accumulate = avr_crc_accumulate,
    .reset = avr_crc_reset,
};
