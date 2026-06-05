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
 * @brief Shared public SPI layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_spi_* API. It hoists the
 * NULL-argument checks (config, data, tx/rx buffers) that both backends
 * duplicated. Instance validity is vendor-specific and stays in the backend.
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
  return _hal_spi_ops.transmit(spi, data, size, timeout);
}

hal_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                             uint16_t size, uint32_t timeout) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_spi_ops.receive(spi, data, size, timeout);
}

hal_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                      const uint8_t *tx_data, uint8_t *rx_data,
                                      uint16_t size, uint32_t timeout) {
  if (tx_data == NULL || rx_data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_spi_ops.transmit_receive(spi, tx_data, rx_data, size, timeout);
}
