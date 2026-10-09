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

#include "common/hal_watchdog.h"
#include "internal/hal_flash_ops.h"
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

/* A backstop, not a timeout: the point is that a flash controller which never
 * clears BSY cannot hang the caller forever. A 128 KiB sector erase can approach
 * 2 s on this part, so the cap is deliberately far beyond any real operation --
 * tripping it means the peripheral is wedged, which is a different failure from
 * "this erase is slow".
 *
 * The watchdog kick is the part that matters in a bootloader. The IWDG keeps
 * running across a system reset and is cleared only by a power-on reset, so a
 * loader inherits whatever timeout the application set -- possibly 100 ms --
 * while the erase it is about to do takes twenty times that. Without a kick in
 * here, every update on a watchdog-enabled board resets mid-erase and comes back
 * to a half-erased partition.
 *
 * The status is ignored on purpose: a kick that reports NOT_INITIALIZED means
 * this build has no watchdog running to feed, which is not this function's
 * problem. See hal_flash_raw_program for the case the driver cannot see.
 */
#define FLASH_BSY_SPINS 40000000UL

static hal_status_t _flash_wait_(void) {
  uint32_t spins = FLASH_BSY_SPINS;
  while (FLASH_SR & FLASH_SR_BSY) {
    if (--spins == 0u)
      return HAL_ERR_TIMEOUT;
#if NAVHAL_CONFIG_DRV_WATCHDOG
    if ((spins & 0xFFFFu) == 0u)
      (void)hal_watchdog_kick();
#endif
  }
  return HAL_OK;
}

static void _flash_unlock_(void) {
  if (FLASH_CR & FLASH_CR_LOCK) {
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
  }
}

static void _flash_lock_(void) { FLASH_CR |= FLASH_CR_LOCK; }

/* Flush the flash accelerator after an erase.
 *
 * RM0368 3.5.1 (F4): the data cache can still hold lines from a sector that has
 * just been erased, so a read of that sector returns what was there before -- the
 * erase succeeded in flash and the reader cannot see it. A sector that held code
 * leaves the instruction cache equally stale, so both are reset. A reset is only
 * permitted while the cache is disabled, which is why this is four writes.
 *
 * The F7 exposes one ART accelerator rather than two caches (RM0410 3.4.1), so
 * there is one enable and one reset bit to work with. Note that the Cortex-M7's
 * own L1 data cache sits above this and is not what these bits touch: on that
 * part a read-after-erase also wants the cache driver, which is why the test
 * that found this is F4-only for now.
 *
 * Prefetch and the wait states are left alone: clock.c owns them, and clearing
 * the latency here would be a far worse bug than the one this fixes.
 *
 * Found by a test that erased a sector and read it back: it saw the pattern it
 * had programmed a step earlier, with the erase reporting success.
 */
static void _flash_cache_flush_(void) {
#if defined(FLASH_ACR_DCEN)
  const uint32_t enables = FLASH_ACR_ICEN | FLASH_ACR_DCEN;
  const uint32_t resets = FLASH_ACR_ICRST | FLASH_ACR_DCRST;
#else
  const uint32_t enables = FLASH_ACR_ARTEN;
  const uint32_t resets = FLASH_ACR_ARTRST;
#endif
  uint32_t keep = FLASH_ACR & ~(enables | resets);
  uint32_t on = FLASH_ACR & enables;

  FLASH_ACR = keep;           /* off: a reset is only valid while disabled */
  FLASH_ACR = keep | resets;  /* reset */
  FLASH_ACR = keep;           /* release */
  FLASH_ACR = keep | on;      /* back on, but only what was on before */
}

static void _flash_erase_sector_(uint8_t sector) {
  _flash_unlock_();
  (void)_flash_wait_();
  FLASH_CR &= ~FLASH_CR_SNB_Msk;
  FLASH_CR |= FLASH_CR_SER | ((sector & 0xF) << FLASH_CR_SNB_Pos);
  FLASH_CR |= FLASH_CR_STRT;
  (void)_flash_wait_();
  FLASH_CR &= ~FLASH_CR_SER;
  _flash_lock_();
  _flash_cache_flush_();
}

static NAVHAL_UNUSED void _flash_program_word_(uint32_t addr, uint32_t data) {
  _flash_unlock_();
  (void)_flash_wait_();
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

  (void)_flash_wait_();
  FLASH_CR &= ~FLASH_CR_PG;
  _flash_lock_();
}

static void _flash_program_half_word_(uint32_t addr, uint16_t data) {
  _flash_unlock_();
  (void)_flash_wait_();
  FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
  FLASH_CR |= (0x1U << FLASH_CR_PSIZE_Pos); // x16 programming
  FLASH_CR |= FLASH_CR_PG;

  *(volatile uint16_t *)addr = data;
  /* Flush the Cortex-M7 write buffer before polling BSY (see the word-program
   * variant above for why this is required). Harmless on Cortex-M4. */
  NAVHAL_FLASH_DSB();

  (void)_flash_wait_();
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

static hal_status_t stm32_flash_save(uint8_t key, const uint8_t *value,
                                     uint8_t size) {
  /* value non-NULL and size != 0: validated by the public layer. */
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

static hal_status_t stm32_flash_read(uint8_t key, uint8_t *value,
                                     uint8_t *size) {
  /* value and size non-NULL: validated by the public layer. The not-found path
   * below stores *size = 0 unguarded, which faults on Cortex-M7 if that ever
   * stops holding. */
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

static hal_status_t stm32_flash_delete(uint8_t key) {
  __IO hal_flash_record_t *rec = _flash_find_first_valid_entry_(key);
  if (rec == NULL)
    return HAL_ERR; // key not found

  hal_flash_record_t updated;
  hal_memcpy(&updated, (const void *)rec, sizeof(hal_flash_record_t));
  updated.status = FLASH_DELETED;
  return _flash_write_data_((uint32_t)rec, (const uint8_t *)&updated,
                            sizeof(hal_flash_record_t));
}

static hal_status_t stm32_flash_erase(void) {
  _flash_erase_sector_(PRIMARY_FLASH_SECTOR);
  _flash_erase_sector_(SECONDARY_FLASH_SECTOR);
  return HAL_OK;
}

static bool stm32_flash_needs_compaction(void) {
  return _flash_find_next_free() == NULL;
}

/* ---- Raw partition access, for a loader writing an image ---------------- */

/* Which sectors a loader may erase or program. Stage-1 is the root of trust and
 * write-protected; the key/value store owns its own two sectors. Everything else
 * -- stage-2 and the application -- is fair game. Derived from the sector map
 * rather than from hal_bootmap.h so this stays true if the partition moves: what
 * matters is the ownership rule, not the addresses. */
static bool _raw_sector_allowed(uint8_t sector) {
  if (sector > 7u)
    return false;
  if (sector <= 1u)
    return false; /* stage-1, WRP'd */
  if (sector == (uint8_t)PRIMARY_FLASH_SECTOR ||
      sector == (uint8_t)SECONDARY_FLASH_SECTOR)
    return false; /* the KV store's, and it erases them itself */
  return true;
}

/* STM32F4 512K map: four 16K, one 64K, three 128K. */
static uint32_t _sector_base(uint8_t sector) {
  static const uint32_t base[8] = {0x08000000UL, 0x08004000UL, 0x08008000UL,
                                   0x0800C000UL, 0x08010000UL, 0x08020000UL,
                                   0x08040000UL, 0x08060000UL};
  return base[sector & 7u];
}

static uint32_t _sector_size(uint8_t sector) {
  static const uint32_t size[8] = {0x4000UL,  0x4000UL,  0x4000UL,  0x4000UL,
                                   0x10000UL, 0x20000UL, 0x20000UL, 0x20000UL};
  return size[sector & 7u];
}

/* Every sector the range touches must be one the caller owns. Checking the whole
 * range rather than its first address is the point: a write that starts inside
 * the app and runs past its end would otherwise be half-accepted. */
static bool _raw_range_allowed(uint32_t addr, uint32_t len) {
  if (len == 0u)
    return false;
  uint32_t end = addr + len; /* exclusive */
  if (end < addr)
    return false; /* wrapped */
  for (uint8_t s = 0u; s < 8u; s++) {
    uint32_t b = _sector_base(s);
    uint32_t e = b + _sector_size(s);
    bool overlaps = (addr < e) && (end > b);
    if (overlaps && !_raw_sector_allowed(s))
      return false;
  }
  /* and it must lie inside the part at all */
  return addr >= _sector_base(0) && end <= (_sector_base(7) + _sector_size(7));
}

hal_status_t hal_flash_raw_erase_sector(uint8_t sector) {
  if (!_raw_sector_allowed(sector))
    return HAL_ERR_INVALID_ARG;

  _flash_unlock_();
  hal_status_t st = _flash_wait_();
  if (st != HAL_OK) {
    _flash_lock_();
    return st;
  }
  FLASH_CR &= ~FLASH_CR_SNB_Msk;
  FLASH_CR |= ((uint32_t)sector << FLASH_CR_SNB_Pos) & FLASH_CR_SNB_Msk;
  FLASH_CR |= FLASH_CR_SER;
  FLASH_CR |= FLASH_CR_STRT;
  NAVHAL_FLASH_DSB();
  st = _flash_wait_();
  FLASH_CR &= ~FLASH_CR_SER;
  _flash_lock_();
  _flash_cache_flush_();
  return st;
}

hal_status_t hal_flash_raw_program(uint32_t addr, const void *data,
                                   uint32_t len) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  if ((addr & 1u) != 0u || (len & 1u) != 0u)
    return HAL_ERR_INVALID_ARG; /* half-word granularity */
  if (!_raw_range_allowed(addr, len))
    return HAL_ERR_INVALID_ARG;

  const uint8_t *src = (const uint8_t *)data;
  _flash_unlock_();
  hal_status_t st = _flash_wait_();
  for (uint32_t off = 0u; st == HAL_OK && off < len; off += 2u) {
    /* The source may be unaligned, so the half-word is assembled by byte. */
    uint16_t hw = (uint16_t)((uint16_t)src[off] | ((uint16_t)src[off + 1u] << 8));
    FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
    FLASH_CR |= (0x1UL << FLASH_CR_PSIZE_Pos); /* 16-bit */
    FLASH_CR |= FLASH_CR_PG;
    *(__IO uint16_t *)(addr + off) = hw;
    NAVHAL_FLASH_DSB();
    st = _flash_wait_();
    FLASH_CR &= ~FLASH_CR_PG;
    if (st == HAL_OK && *(__IO uint16_t *)(addr + off) != hw)
      st = HAL_ERR_IO; /* programmed, did not read back */
  }
  _flash_lock_();
  return st;
}

const hal_flash_ops_t _hal_flash_ops = {
    .save = stm32_flash_save,
    .read = stm32_flash_read,
    .del = stm32_flash_delete,
    .erase = stm32_flash_erase,
    .needs_compaction = stm32_flash_needs_compaction,
};
