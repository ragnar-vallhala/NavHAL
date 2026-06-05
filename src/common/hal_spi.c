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
 * @file src/common/hal_spi.c
 * @brief Shared public SPI layer over the per-vendor hardware primitives.
 *
 * @details
 * The vendor backends provide two primitives (::hal_spi_ops_t — init and
 * xfer_byte). The three blocking transfer directions are implemented here,
 * once, as loops over xfer_byte:
 *
 *   - transmit         — clock out @p data, discard what comes back,
 *   - receive          — clock out 0xFF, keep what comes back,
 *   - transmit_receive — clock out @p tx_data, keep @p rx_data.
 *
 * The @c timeout argument is accepted for API compatibility but is a no-op: a
 * stuck transfer is bounded by xfer_byte's internal iteration guard. (The AVR
 * backend already ignored the millisecond timeout; this makes both ports
 * behave the same and removes the timebase dependency from the SPI path.)
 */

#include "common/hal_spi.h"
#include "internal/hal_spi_ops.h"

#include <stddef.h>

hal_status_t hal_spi_init(hal_spi_instance_t spi,
                          const hal_spi_config_t *config) {
  if (config == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_spi_ops.init(spi, config);
}

hal_status_t hal_spi_transmit(hal_spi_instance_t spi, const uint8_t *data,
                              uint16_t size, uint32_t timeout) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  (void)timeout;
  for (uint16_t i = 0; i < size; i++) {
    hal_status_t s = _hal_spi_ops.xfer_byte(spi, data[i], NULL);
    if (s != HAL_OK)
      return s;
  }
  return HAL_OK;
}

hal_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                             uint16_t size, uint32_t timeout) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  (void)timeout;
  for (uint16_t i = 0; i < size; i++) {
    hal_status_t s = _hal_spi_ops.xfer_byte(spi, 0xFFu, &data[i]);
    if (s != HAL_OK)
      return s;
  }
  return HAL_OK;
}

hal_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                      const uint8_t *tx_data, uint8_t *rx_data,
                                      uint16_t size, uint32_t timeout) {
  if (tx_data == NULL || rx_data == NULL)
    return HAL_ERR_INVALID_ARG;
  (void)timeout;
  for (uint16_t i = 0; i < size; i++) {
    hal_status_t s = _hal_spi_ops.xfer_byte(spi, tx_data[i], &rx_data[i]);
    if (s != HAL_OK)
      return s;
  }
  return HAL_OK;
}
