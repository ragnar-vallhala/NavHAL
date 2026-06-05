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
 * @file internal/hal_flash_ops.h
 * @brief HAL-internal Flash vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_flash.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_flash.c) dispatches through. See @c internal/hal_gpio_ops.h
 * for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_FLASH_OPS_H
#define NAVHAL_INTERNAL_HAL_FLASH_OPS_H

#include "common/hal_flash.h"
#include "common/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend Flash key/value operations table. */
typedef struct {
  /** Backend for ::hal_flash_save. NULL value / zero size rejected upstream. */
  hal_status_t (*save)(uint8_t key, const uint8_t *value, uint8_t size);
  /** Backend for ::hal_flash_read. NULL value / size rejected upstream. */
  hal_status_t (*read)(uint8_t key, uint8_t *value, uint8_t *size);
  /** Backend for ::hal_flash_delete. (@c del — @c delete is a C++ keyword.) */
  hal_status_t (*del)(uint8_t key);
  /** Backend for ::hal_flash_erase. */
  hal_status_t (*erase)(void);
  /** Backend for ::hal_flash_needs_compaction. */
  bool (*needs_compaction)(void);
} hal_flash_ops_t;

/** @brief The active port's Flash operations table (defined by one backend). */
extern const hal_flash_ops_t _hal_flash_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_FLASH_OPS_H */
