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
 * @file spi_types.h
 * @brief SPI peripheral-instance identifier — Cortex-M4 / STM32F4 port.
 *
 * @details
 * The set of valid SPI instances is target-defined, so ::hal_spi_instance_t
 * is port-resolved: each port ships its own @c utils/spi_types.h selected by
 * the `-I include/port/<processor>` path. Portable SPI concepts that are not
 * target-shaped (baud-rate prescaler, polarity, phase, data size, bit order)
 * stay in @c common/hal_spi.h.
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI peripheral instance identifier (STM32F401RE: SPI1/2).
 */
typedef enum {
  HAL_SPI_1 = 1, /**< SPI instance 1. */
  HAL_SPI_2 = 2, /**< SPI instance 2. */
  SPI_1 NAVHAL_DEPRECATED("use HAL_SPI_1") = 1,
  SPI_2 NAVHAL_DEPRECATED("use HAL_SPI_2") = 2,
} hal_spi_instance_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SPI_TYPES_H */
