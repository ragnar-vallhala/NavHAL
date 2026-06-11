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
 * @file hal_flash.h
 * @brief Portable HAL interface for Flash key/value storage.
 *
 * @details
 * Standardized Flash API (see @c docs/api_standardization.md). Provides a
 * simple key/value store backed by the on-chip Flash, with wear-levelling
 * compaction between a primary and secondary sector. All functions use the
 * @c hal_flash_ prefix and return ::hal_status_t.
 */

#ifndef HAL_FLASH_H
#define HAL_FLASH_H

/**
 * @defgroup HAL_FLASH Flash
 * @ingroup HAL_DRIVERS
 * @brief Embedded-flash read / erase / program.
 * @{
 */

#include "common/hal_status.h"
#include "common/hal_types.h"
#include "common/navhal_compiler.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief On-flash key/value record header.
 */
typedef struct hal_flash_record {
  uint8_t magic;    /**< Record magic marker. */
  uint8_t key;      /**< User key. */
  uint8_t size;     /**< Length of the stored value. */
  uint8_t status;   /**< 0xFF = empty, 0x01 = valid, 0x00 = deleted. */
  uint8_t reserved; /**< Alignment padding. */
  uint8_t crc;      /**< XOR checksum of the value. */
} hal_flash_record_t;

/**
 * @brief Store a value under a key (supersedes any previous value).
 * @param key   User key.
 * @param value Value bytes.
 * @param size  Value length in bytes (non-zero).
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_flash_save(uint8_t key, const uint8_t *value, uint8_t size);

/**
 * @brief Read the value stored under a key.
 * @param key   User key.
 * @param value Destination buffer.
 * @param size  Out: number of bytes written to @p value.
 * @return ::HAL_OK, or ::HAL_ERR if the key is not found / CRC mismatch.
 */
hal_status_t hal_flash_read(uint8_t key, uint8_t *value, uint8_t *size);

/**
 * @brief Delete the value stored under a key.
 * @param key User key.
 * @return ::HAL_OK, or ::HAL_ERR if the key is not found.
 */
hal_status_t hal_flash_delete(uint8_t key);

/**
 * @brief Erase the entire key/value storage area.
 * @return ::HAL_OK.
 */
hal_status_t hal_flash_erase(void);

/**
 * @brief Report whether the storage area is full and needs compaction.
 * @return true if compaction is needed, false otherwise.
 */
bool hal_flash_needs_compaction(void);

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization Flash type names. The status enum is now
 * ::hal_status_t. Retained as a backward-compat alias.
 * -------------------------------------------------------------------------- */
typedef hal_flash_record_t FlashRecord_t
    NAVHAL_DEPRECATED("use hal_flash_record_t");
typedef hal_status_t FlashStatus_t NAVHAL_DEPRECATED("use hal_status_t");
#define FLASH_OK HAL_OK                  /**< @deprecated HAL_OK */
#define FLASH_ERR_NOT_FOUND HAL_ERR      /**< @deprecated HAL_ERR */
#define FLASH_ERR_WRITE HAL_ERR_IO       /**< @deprecated HAL_ERR_IO */
#define FLASH_ERR_ERASE HAL_ERR_IO       /**< @deprecated HAL_ERR_IO */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits (compat shim with deprecated function names). */
#include "navhal_port_flash.h"


/** @} */ /* end of group HAL_FLASH */
#endif /* HAL_FLASH_H */
