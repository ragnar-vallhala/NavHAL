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
 * @file flash_compat.h
 * @brief Deprecated pre-standardization Flash API shim.
 *
 * @details
 * Maps the pre-standardization Flash key/value function names onto the
 * standardized `hal_flash_*` API. Each wrapper is marked deprecated, so legacy
 * calls emit a compiler warning naming the replacement. Included automatically
 * by `port/cortex-m4/navhal_port_flash.h`.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 */

#ifndef NAVHAL_FLASH_COMPAT_H
#define NAVHAL_FLASH_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_flash_save(). */
NAVHAL_DEPRECATED("use hal_flash_save")
static inline hal_status_t save_data_to_flash(uint8_t key,
                                              const uint8_t *value,
                                              uint8_t size) {
  return hal_flash_save(key, value, size);
}

/** @deprecated Use hal_flash_read(). */
NAVHAL_DEPRECATED("use hal_flash_read")
static inline hal_status_t read_data_from_flash(uint8_t key, uint8_t *value,
                                                uint8_t *size) {
  return hal_flash_read(key, value, size);
}

/** @deprecated Use hal_flash_delete(). */
NAVHAL_DEPRECATED("use hal_flash_delete")
static inline hal_status_t delete_data_from_flash(uint8_t key) {
  return hal_flash_delete(key);
}

/** @deprecated Use hal_flash_erase(). */
NAVHAL_DEPRECATED("use hal_flash_erase")
static inline hal_status_t flash_storage_erase(void) {
  return hal_flash_erase();
}

/** @deprecated Use hal_flash_needs_compaction(). */
NAVHAL_DEPRECATED("use hal_flash_needs_compaction")
static inline int flash_storage_needs_compaction(void) {
  return hal_flash_needs_compaction() ? 1 : 0;
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_FLASH_COMPAT_H */
