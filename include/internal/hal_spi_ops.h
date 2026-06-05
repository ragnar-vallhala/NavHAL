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
 * @file internal/hal_spi_ops.h
 * @brief HAL-internal SPI vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_spi.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_spi.c) dispatches through. See @c internal/hal_gpio_ops.h
 * for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_SPI_OPS_H
#define NAVHAL_INTERNAL_HAL_SPI_OPS_H

#include "common/hal_spi.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend SPI operations table. */
typedef struct {
  /** Backend for ::hal_spi_init. NULL @p config rejected upstream. */
  hal_status_t (*init)(hal_spi_instance_t spi, const hal_spi_config_t *config);
  /** Backend for ::hal_spi_transmit. NULL @p data rejected upstream. */
  hal_status_t (*transmit)(hal_spi_instance_t spi, const uint8_t *data,
                           uint16_t size, uint32_t timeout);
  /** Backend for ::hal_spi_receive. NULL @p data rejected upstream. */
  hal_status_t (*receive)(hal_spi_instance_t spi, uint8_t *data, uint16_t size,
                          uint32_t timeout);
  /** Backend for ::hal_spi_transmit_receive. NULL tx/rx buffer rejected upstream. */
  hal_status_t (*transmit_receive)(hal_spi_instance_t spi,
                                   const uint8_t *tx_data, uint8_t *rx_data,
                                   uint16_t size, uint32_t timeout);
} hal_spi_ops_t;

/** @brief The active port's SPI operations table (defined by one backend). */
extern const hal_spi_ops_t _hal_spi_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_SPI_OPS_H */
