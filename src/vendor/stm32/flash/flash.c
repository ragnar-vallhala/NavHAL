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
 * @file flash.c
 * @brief Standardized HAL Flash key/value storage driver for STM32F4.
 *
 * @details
 * Implements the standardized `hal_flash_*` API declared in
 * `port/cortex-m4/navhal_port_flash.h`: a simple key/value store backed by the on-chip
 * Flash, with compaction between a primary and secondary sector.
 */

#include "navhal_port_flash.h"
#include "common/hal_types.h"
#include "family/flash_reg.h"
#include "utils/util.h"
#include <stddef.h>
#include <stdint.h>

/* Data synchronization barrier — flush the Cortex-M7 write buffer so a flash
 * store reaches the controller before BSY is polled. Portable to the host
 * driver test build (where there is no flash controller to order against). */
#if defined(__arm__) || defined(__thumb__)
#define NAVHAL_FLASH_DSB() __asm volatile("dsb 0xF" ::: "memory")
#else
#define NAVHAL_FLASH_DSB() __atomic_signal_fence(__ATOMIC_SEQ_CST)
#endif

/* ---- Internal low-level flash primitives -------------------------------- */

static void _flash_wait_(void) {
  while (FLASH_SR & FLASH_SR_BSY)
    ;
}

static void _flash_unlock_(void) {
  if (FLASH_CR & FLASH_CR_LOCK) {
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
  }
}

static void _flash_lock_(void) { FLASH_CR |= FLASH_CR_LOCK; }

static void _flash_erase_sector_(uint8_t sector) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_SNB_Msk;
  FLASH_CR |= FLASH_CR_SER | ((sector & 0xF) << FLASH_CR_SNB_Pos);
  FLASH_CR |= FLASH_CR_STRT;
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_SER;
  _flash_lock_();
}

static NAVHAL_UNUSED void _flash_program_word_(uint32_t addr, uint32_t data) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
  FLASH_CR |= (0x2U << FLASH_CR_PSIZE_Pos); // x32 programming
  FLASH_CR |= FLASH_CR_PG;

  *(volatile uint32_t *)addr = data;
  /* The store to flash (Normal memory) can sit in the Cortex-M7 write buffer;
   * without a barrier, _flash_wait_ reads BSY before the program has started,
   * returns immediately, and PG is cleared before the write commits — the
   * write is silently lost. A DSB forces the store to reach the flash
   * controller first. Harmless on Cortex-M4. */
  NAVHAL_FLASH_DSB();

  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PG;
  _flash_lock_();
}

static void _flash_program_half_word_(uint32_t addr, uint16_t data) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
  FLASH_CR |= (0x1U << FLASH_CR_PSIZE_Pos); // x16 programming
  FLASH_CR |= FLASH_CR_PG;

  *(volatile uint16_t *)addr = data;
  /* Flush the Cortex-M7 write buffer before polling BSY (see the word-program
   * variant above for why this is required). Harmless on Cortex-M4. */
  NAVHAL_FLASH_DSB();

  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PG;
  _flash_lock_();
}

static NAVHAL_UNUSED uint32_t _flash_read_word_(uint32_t addr) {
  return *(volatile uint32_t *)addr;
}

static uint16_t _flash_read_half_word_(uint32_t addr) {
  return *(volatile uint16_t *)addr;
}

static uint8_t _flash_calculate_crc_(const uint8_t *value, uint8_t size) {
  if (size == 0)
    return 0;
  uint8_t crc = value[0];
  for (int i = 1; i < size; i++) {
    crc ^= value[i];
  }
  return crc;
}

/* ---- Internal record / storage helpers ---------------------------------- */

static __IO uint8_t *_flash_find_next_free(void) {
  __IO uint8_t *ptr = (__IO uint8_t *)FLASH_PRIMARY_STORAGE_START;
  hal_flash_record_t *rec = (hal_flash_record_t *)ptr;
  while (rec->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr < FLASH_PRIMARY_STORAGE_END) {
    ptr = ptr + rec->size + sizeof(hal_flash_record_t);
    rec = (hal_flash_record_t *)ptr;
  }
  if ((uint32_t)ptr >= FLASH_PRIMARY_STORAGE_END)
    return NULL;
  return ptr;
}

static __IO hal_flash_record_t *_flash_find_first_valid_entry_(uint8_t key) {

  __IO hal_flash_record_t *rec =
      (__IO hal_flash_record_t *)FLASH_PRIMARY_STORAGE_START;
  while ((uint32_t)rec < FLASH_PRIMARY_STORAGE_END &&
         rec->magic == FLASH_MAGIC_NUMBER) {
    if (rec->key == key && rec->status == FLASH_VALID)
      return rec;
    uint8_t *byte_addr = (uint8_t *)rec;
    byte_addr = byte_addr + rec->size + sizeof(hal_flash_record_t);
    rec = (hal_flash_record_t *)byte_addr;
  }
  return NULL;
}

static hal_status_t _flash_write_data_(uint32_t addr, const uint8_t *data,
                                       uint8_t size) {
  if (size == 0)
    return HAL_ERR_IO;

  uint8_t padded_size = (size % 2 == 0) ? size : size + 1;
  uint16_t half_word = 0;

  for (uint8_t i = 0; i < padded_size; i += 2) {
    if (i + 1 < size) {
      // Normal case: 2 valid bytes
      half_word = (data[i + 1] << 8) | data[i];
    } else {
      // Last odd byte — pad with 0xFF (or 0x00 if you prefer)
      half_word = (0xFF << 8) | data[i];
    }
    _flash_program_half_word_(addr, half_word);
    addr += 2;
  }

  return HAL_OK;
}

static NAVHAL_UNUSED hal_status_t _flash_read_data_(uint32_t addr,
                                                    uint8_t *data,
                                                    uint8_t size) {
  if (size == 0)
    return HAL_ERR;

  uint8_t padded_size = (size % 2 == 0) ? size : size + 1;
  uint16_t half_word = 0;
  for (uint8_t i = 0; i < padded_size; i += 2) {
    half_word = _flash_read_half_word_(addr);
    if (i + 1 < size) {
      // Normal case: 2 valid bytes
      data[i] = half_word & (0xFF);
      data[i + 1] = (half_word >> 8) & (0xFF);
    } else {
      data[i] = half_word & (0xFF);
    }

    addr += 2;
  }
  return HAL_OK;
}

static hal_status_t _flash_shift_sector_primary_to_secondary_(void) {
  uint8_t *ptr_primary = (uint8_t *)FLASH_PRIMARY_STORAGE_START;
  uint8_t *ptr_secondary = (uint8_t *)FLASH_SECONDARY_STORAGE_START;
  hal_flash_record_t *rec_primary = (hal_flash_record_t *)ptr_primary;
  while (rec_primary->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr_primary < FLASH_PRIMARY_STORAGE_END) {
    if (rec_primary->status != FLASH_VALID) {
      uint8_t total_size = sizeof(hal_flash_record_t) + rec_primary->size;
      ptr_primary += total_size;
      rec_primary = (hal_flash_record_t *)ptr_primary;
      continue;
    }
    uint8_t total_size = sizeof(hal_flash_record_t) + rec_primary->size;
    hal_status_t status =
        _flash_write_data_((uint32_t)ptr_secondary, ptr_primary, total_size);
    if (status != HAL_OK)
      return status;
    ptr_primary += total_size;
    ptr_secondary += total_size;
    rec_primary = (hal_flash_record_t *)ptr_primary;
  }

  return HAL_OK;
}

static hal_status_t _flash_shift_sector_secondary_to_primary_(void) {
  uint8_t *ptr_primary = (uint8_t *)FLASH_PRIMARY_STORAGE_START;
  uint8_t *ptr_secondary = (uint8_t *)FLASH_SECONDARY_STORAGE_START;
  hal_flash_record_t *rec_secondary = (hal_flash_record_t *)ptr_secondary;
  while (rec_secondary->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr_secondary < FLASH_SECONDARY_STORAGE_END) {
    uint8_t total_size = sizeof(hal_flash_record_t) + rec_secondary->size;
    hal_status_t status =
        _flash_write_data_((uint32_t)ptr_primary, ptr_secondary, total_size);
    if (status != HAL_OK)
      return status;
    ptr_primary += total_size;
    ptr_secondary += total_size;
    rec_secondary = (hal_flash_record_t *)ptr_secondary;
  }
  return HAL_OK;
}

static hal_status_t _flash_compact_storage_(void) {
  hal_status_t status;
  status = _flash_shift_sector_primary_to_secondary_();
  if (status != HAL_OK)
    return status;
  _flash_erase_sector_(PRIMARY_FLASH_SECTOR);
  status = _flash_shift_sector_secondary_to_primary_();
  if (status != HAL_OK)
    return status;
  _flash_erase_sector_(SECONDARY_FLASH_SECTOR);
  return HAL_OK;
}

/* ---- Public API --------------------------------------------------------- */

hal_status_t hal_flash_save(uint8_t key, const uint8_t *value, uint8_t size) {
  if (size == 0 || value == NULL)
    return HAL_ERR;

  __IO uint8_t *ptr = _flash_find_next_free();
  if (ptr == NULL) {
    hal_status_t status = _flash_compact_storage_();
    if (status != HAL_OK)
      return status;
    ptr = _flash_find_next_free();
    if (ptr == NULL)
      return HAL_ERR_IO;
  }

  __IO hal_flash_record_t *last_rec = _flash_find_first_valid_entry_(key);
  if (last_rec != NULL) {
    hal_flash_record_t updated_last_rec;
    hal_memcpy(&updated_last_rec, (const void *)last_rec,
               sizeof(hal_flash_record_t));
    updated_last_rec.status = FLASH_DELETED;
    hal_status_t status = _flash_write_data_(
        (uint32_t)(last_rec), (const uint8_t *)&updated_last_rec,
        sizeof(hal_flash_record_t));
    if (status != HAL_OK)
      return status;
  }

  hal_flash_record_t rec;
  rec.key = key;
  rec.magic = FLASH_MAGIC_NUMBER;
  rec.size = size;
  rec.status = FLASH_VALID;
  rec.crc = _flash_calculate_crc_(value, size);
  hal_status_t status = _flash_write_data_(
      (uint32_t)ptr, (const uint8_t *)&rec, sizeof(hal_flash_record_t));
  if (status != HAL_OK)
    return status;
  ptr += (sizeof(hal_flash_record_t) % 2 == 0 ? sizeof(hal_flash_record_t)
                                              : sizeof(hal_flash_record_t) + 1);
  status = _flash_write_data_((uint32_t)ptr, value, size);
  return status;
}

hal_status_t hal_flash_read(uint8_t key, uint8_t *value, uint8_t *size) {
  /* NULL-guard before any dereference. Without this, a NULL `size` made the
   * not-found path below write `*size = 0` to address 0 — harmless on the
   * Cortex-M4 / Renode flash model, but a faulting store on real Cortex-M7. */
  if (value == NULL || size == NULL)
    return HAL_ERR_INVALID_ARG;

  __IO hal_flash_record_t *last_rec = _flash_find_first_valid_entry_(key);
  if (last_rec == NULL) {
    *size = 0;
    return HAL_ERR;
  }
  uint8_t *src = ((uint8_t *)last_rec) + sizeof(hal_flash_record_t);
  *size = last_rec->size;
  hal_memcpy(value, src, *size);
  if (last_rec->crc != _flash_calculate_crc_(value, *size))
    return HAL_ERR;
  return HAL_OK;
}

hal_status_t hal_flash_delete(uint8_t key) {
  __IO hal_flash_record_t *rec = _flash_find_first_valid_entry_(key);
  if (rec == NULL)
    return HAL_ERR; // key not found

  hal_flash_record_t updated;
  hal_memcpy(&updated, (const void *)rec, sizeof(hal_flash_record_t));
  updated.status = FLASH_DELETED;
  return _flash_write_data_((uint32_t)rec, (const uint8_t *)&updated,
                            sizeof(hal_flash_record_t));
}

hal_status_t hal_flash_erase(void) {
  _flash_erase_sector_(PRIMARY_FLASH_SECTOR);
  _flash_erase_sector_(SECONDARY_FLASH_SECTOR);
  return HAL_OK;
}

bool hal_flash_needs_compaction(void) {
  return _flash_find_next_free() == NULL;
}
