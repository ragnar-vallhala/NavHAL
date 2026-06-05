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
 * @brief HAL-internal SPI vendor-backend interface (hardware primitives).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_spi.h.
 *
 * SPI is full-duplex: every transfer is a sequence of single-byte exchanges.
 * The vtable therefore exposes just two primitives — configure the peripheral
 * and exchange one byte — and the shared public layer @c src/common/hal_spi.c
 * builds transmit / receive / transmit_receive as loops over @c xfer_byte. A
 * new SPI port supplies these two and inherits all three transfer directions.
 *
 * See @c internal/hal_gpio_ops.h for the embedded-table / LTO rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_SPI_OPS_H
#define NAVHAL_INTERNAL_HAL_SPI_OPS_H

#include "common/hal_spi.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend SPI hardware primitives. */
typedef struct {
  /** Configure the SPI master. NULL @p config rejected upstream. */
  hal_status_t (*init)(hal_spi_instance_t spi, const hal_spi_config_t *config);
  /**
   * @brief Exchange one byte full-duplex (blocking).
   * @param out Byte clocked out on MOSI.
   * @param in  Receives the byte clocked in on MISO; may be NULL to discard.
   * @return ::HAL_OK; ::HAL_ERR_INVALID_ARG for an invalid instance;
   *         ::HAL_ERR_TIMEOUT if the peripheral never becomes ready (an
   *         internal iteration guard bounds a stuck transfer).
   */
  hal_status_t (*xfer_byte)(hal_spi_instance_t spi, uint8_t out, uint8_t *in);
} hal_spi_ops_t;

/** @brief The active port's SPI primitives (defined by one backend). */
extern const hal_spi_ops_t _hal_spi_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_SPI_OPS_H */
