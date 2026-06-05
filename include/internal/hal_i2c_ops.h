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
 * @file internal/hal_i2c_ops.h
 * @brief HAL-internal I2C vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_i2c.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_i2c.c) dispatches through. Port-specific extensions (e.g.
 * the STM32 DMA register-read helper in @c navhal_port_i2c.h) stay outside it.
 * See @c internal/hal_gpio_ops.h for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_I2C_OPS_H
#define NAVHAL_INTERNAL_HAL_I2C_OPS_H

#include "common/hal_i2c.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend I2C operations table (portable API only). */
typedef struct {
  /** Backend for ::hal_i2c_init. NULL @p config rejected upstream. */
  hal_status_t (*init)(hal_i2c_bus_t bus, const hal_i2c_config_t *config);
  /** Backend for ::hal_i2c_write. NULL @p data rejected upstream. */
  hal_status_t (*write)(hal_i2c_bus_t bus, uint8_t dev_addr,
                        const uint8_t *data, uint16_t len);
  /** Backend for ::hal_i2c_read. NULL @p data rejected upstream. */
  hal_status_t (*read)(hal_i2c_bus_t bus, uint8_t dev_addr, uint8_t *data,
                       uint16_t len);
  /** Backend for ::hal_i2c_write_read. NULL tx/rx buffer rejected upstream. */
  hal_status_t (*write_read)(hal_i2c_bus_t bus, uint8_t dev_addr,
                             const uint8_t *tx_data, uint16_t tx_len,
                             uint8_t *rx_data, uint16_t rx_len);
  /** Backend for ::hal_i2c_get_init_status. */
  uint8_t (*get_init_status)(void);
} hal_i2c_ops_t;

/** @brief The active port's I2C operations table (defined by one backend). */
extern const hal_i2c_ops_t _hal_i2c_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_I2C_OPS_H */
