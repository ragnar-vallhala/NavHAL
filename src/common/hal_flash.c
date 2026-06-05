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
 * @file src/common/hal_flash.c
 * @brief Shared public Flash layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_flash_* API. It establishes the
 * canonical argument contract that the vendor backends previously disagreed on:
 * a NULL value or zero size on ::hal_flash_save, and a NULL value or size
 * pointer on ::hal_flash_read, are rejected with ::HAL_ERR_INVALID_ARG before
 * any backend runs. (Previously the AVR/EEPROM backend rejected these while the
 * STM32 backend did not — exactly the silent-divergence M9 removes.) Backends
 * keep only the register/EEPROM work.
 */

#include "common/hal_flash.h"
#include "internal/hal_flash_ops.h"

#include <stddef.h>

hal_status_t hal_flash_save(uint8_t key, const uint8_t *value, uint8_t size) {
  if (value == NULL || size == 0)
    return HAL_ERR_INVALID_ARG;
  return _hal_flash_ops.save(key, value, size);
}

hal_status_t hal_flash_read(uint8_t key, uint8_t *value, uint8_t *size) {
  if (value == NULL || size == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_flash_ops.read(key, value, size);
}

hal_status_t hal_flash_delete(uint8_t key) { return _hal_flash_ops.del(key); }

hal_status_t hal_flash_erase(void) { return _hal_flash_ops.erase(); }

bool hal_flash_needs_compaction(void) {
  return _hal_flash_ops.needs_compaction();
}
