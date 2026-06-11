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
 * @file crc_types.h
 * @brief CRC type definitions for NavHAL.
 *
 * Defines enumerations and configuration structures shared between
 * the common CRC HAL interface and the architecture-specific driver.
 */

#ifndef CRC_TYPES_H
#define CRC_TYPES_H

/**
 * @defgroup HAL_UTIL_CRC_TYPES Crc Types
 * @ingroup HAL_UTILS
 * @brief CRC polynomial / parameter type definitions.
 * @{
 */

#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @enum hal_crc_polynomial_t
 * @brief Supported CRC polynomial presets.
 *
 * Only CRC-32/MPEG-2 (poly 0x04C11DB7) is exposed for now because that is
 * what the STM32F4 hardware unit natively computes. Extend this enum when
 * support for additional polynomials is added.
 */
typedef enum {
  HAL_CRC_POLY_CRC32 = 0, /**< CRC-32/MPEG-2  poly=0x04C11DB7  init=0xFFFFFFFF */
  CRC_POLY_CRC32 NAVHAL_DEPRECATED("use HAL_CRC_POLY_CRC32") = 0,
} hal_crc_polynomial_t;

/**
 * @struct hal_crc_config_t
 * @brief CRC module configuration.
 *
 * Pass a fully populated instance to hal_crc_init().
 */
typedef struct {
  hal_crc_polynomial_t polynomial; /**< Polynomial preset to use. */
  uint32_t init_value;             /**< Initial accumulator value. */
} hal_crc_config_t;

/* Deprecated pre-standardization CRC type names — retained as a backward-compat alias. */
typedef hal_crc_polynomial_t crc_polynomial_t
    NAVHAL_DEPRECATED("use hal_crc_polynomial_t");
typedef hal_crc_config_t crc_config_t NAVHAL_DEPRECATED("use hal_crc_config_t");


#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_UTIL_CRC_TYPES */
#endif /* CRC_TYPES_H */
