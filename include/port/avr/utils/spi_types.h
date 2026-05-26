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
 * @brief SPI peripheral-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single SPI peripheral, exposed as ::HAL_SPI_0.
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief SPI peripheral instance identifier (ATmega328P: the SPI unit). */
typedef enum {
  HAL_SPI_0 = 0, /**< SPI — the only SPI peripheral on the ATmega328P. */
} hal_spi_instance_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SPI_TYPES_H */
