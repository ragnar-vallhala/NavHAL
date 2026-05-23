/**
 * @file src/vendor/microchip/crc/crc.c
 * @brief ATmega328P CRC HAL driver — software CRC-32/MPEG-2.
 *
 * @details
 * Implements @c common/hal_crc.h for the ATmega328P. The ATmega328P has no
 * hardware CRC unit, so this is a bitwise software implementation of
 * CRC-32/MPEG-2 (polynomial 0x04C11DB7, non-reflected, no final XOR) — the
 * variant @c utils/crc_types.h documents, so the result matches the STM32F4
 * hardware unit. A bitwise loop is used rather than a 1 KiB lookup table to
 * keep the flash footprint small.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_crc.h"

#include <stddef.h>

/** @brief Configured init value, applied by ::hal_crc_reset. */
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

hal_status_t hal_crc_init(const hal_crc_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  s_init = cfg->init_value;
  s_acc = s_init;
  return HAL_OK;
}

hal_status_t hal_crc_reset(void) {
  s_acc = s_init;
  return HAL_OK;
}

uint32_t hal_crc_accumulate(const uint8_t *data, uint32_t len) {
  if (data != NULL)
    s_acc = crc32_mpeg2(s_acc, data, len);
  return s_acc;
}

uint32_t hal_crc_compute(const uint8_t *data, uint32_t len) {
  s_acc = s_init;
  return hal_crc_accumulate(data, len);
}
