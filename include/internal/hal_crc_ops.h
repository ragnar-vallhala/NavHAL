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
 * @file internal/hal_crc_ops.h
 * @brief HAL-internal CRC vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_crc.h.
 * Declares the per-backend operations table that the shared public layer
 * (@c src/common/hal_crc.c) validates and dispatches through. See
 * @c internal/hal_gpio_ops.h for the rationale behind embedding the table
 * directly (a @c const object, devirtualised under @c -flto).
 */

#ifndef NAVHAL_INTERNAL_HAL_CRC_OPS_H
#define NAVHAL_INTERNAL_HAL_CRC_OPS_H

#include "common/hal_crc.h"
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend CRC operations table. */
typedef struct {
  /** Backend for ::hal_crc_init. Validation (NULL @p cfg) has already run. */
  hal_status_t (*init)(const hal_crc_config_t *cfg);
  /** Backend for ::hal_crc_compute. */
  uint32_t (*compute)(const uint8_t *data, uint32_t len);
  /** Backend for ::hal_crc_accumulate. */
  uint32_t (*accumulate)(const uint8_t *data, uint32_t len);
  /** Backend for ::hal_crc_reset. */
  hal_status_t (*reset)(void);
} hal_crc_ops_t;

/** @brief The active port's CRC operations table (defined by one backend). */
extern const hal_crc_ops_t _hal_crc_ops;

/* Portable software CRC-32/MPEG-2 backend (implemented in src/common/hal_crc.c).
 * A port without a hardware CRC unit points its ops table at these instead of
 * carrying its own copy of the algorithm. */
hal_status_t hal_crc_sw_init(const hal_crc_config_t *cfg);
hal_status_t hal_crc_sw_reset(void);
uint32_t hal_crc_sw_accumulate(const uint8_t *data, uint32_t len);
uint32_t hal_crc_sw_compute(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_CRC_OPS_H */
