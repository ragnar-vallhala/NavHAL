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
 * @file src/common/hal_i2c.c
 * @brief Shared public I2C layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_i2c_* API. It hoists the
 * NULL-argument checks (config, data, tx/rx buffers) — which the STM32 backend
 * previously omitted (dereferencing a NULL config) while the AVR backend
 * enforced. Bus validity, reinit detection, and length guards are
 * vendor-specific and stay in the backend.
 */

#include "common/hal_i2c.h"
#include "internal/hal_i2c_ops.h"

#include <stddef.h>

hal_status_t hal_i2c_init(hal_i2c_bus_t bus, const hal_i2c_config_t *config) {
  if (config == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_i2c_ops.init(bus, config);
}

hal_status_t hal_i2c_deinit(hal_i2c_bus_t bus) {
  return _hal_i2c_ops.deinit(bus);
}

hal_status_t hal_i2c_write(hal_i2c_bus_t bus, uint8_t dev_addr,
                           const uint8_t *data, uint16_t len) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_i2c_ops.write(bus, dev_addr, data, len);
}

hal_status_t hal_i2c_read(hal_i2c_bus_t bus, uint8_t dev_addr, uint8_t *data,
                          uint16_t len) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_i2c_ops.read(bus, dev_addr, data, len);
}

hal_status_t hal_i2c_write_read(hal_i2c_bus_t bus, uint8_t dev_addr,
                                const uint8_t *tx_data, uint16_t tx_len,
                                uint8_t *rx_data, uint16_t rx_len) {
  if (tx_data == NULL || rx_data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_i2c_ops.write_read(bus, dev_addr, tx_data, tx_len, rx_data,
                                 rx_len);
}

uint8_t hal_i2c_get_init_status(void) {
  return _hal_i2c_ops.get_init_status();
}
