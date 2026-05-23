/**
 * @file src/vendor/microchip/flash/flash.c
 * @brief ATmega328P key/value storage HAL driver — EEPROM-backed.
 *
 * @details
 * Implements @c common/hal_flash.h for the ATmega328P. Self-programming the
 * program flash from the application is awkward and bootloader-dependent, so
 * this driver backs the key/value store with the 1 KiB on-chip EEPROM
 * instead — byte-addressable and a natural fit for a small KV store.
 *
 * Records are packed sequentially: a 6-byte header (::hal_flash_record_t)
 * followed by the value bytes. A superseded or deleted key has its header
 * status set to 0x00; ::hal_flash_save appends a fresh record. The first
 * slot whose magic byte is not ::KV_MAGIC marks the end of the store.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_flash.h"

#include <avr/eeprom.h>
#include <stddef.h>

#define KV_MAGIC 0xA5u   /**< Header magic marker. */
#define KV_VALID 0x01u   /**< Record status: live. */
#define KV_DELETED 0x00u /**< Record status: superseded / deleted. */
#define KV_HDR 6u        /**< On-EEPROM header size (== sizeof record). */
#define KV_SIZE ((uint16_t)E2END + 1u) /**< EEPROM size (1024 B). */

static uint8_t ee_get(uint16_t off) {
  return eeprom_read_byte((const uint8_t *)off);
}

static void ee_put(uint16_t off, uint8_t val) {
  eeprom_update_byte((uint8_t *)off, val);
}

/** @brief Offset of the (key, status) record, or 0xFFFF if not present. */
static uint16_t kv_find(uint8_t key, uint8_t status) {
  uint16_t off = 0;
  while (off + KV_HDR <= KV_SIZE) {
    if (ee_get(off) != KV_MAGIC)
      break; /* free space — end of the store. */
    if (ee_get(off + 1u) == key && ee_get(off + 3u) == status)
      return off;
    off = (uint16_t)(off + KV_HDR + ee_get(off + 2u));
  }
  return 0xFFFFu;
}

/** @brief Offset of the first free slot (end of the record chain). */
static uint16_t kv_end(void) {
  uint16_t off = 0;
  while (off + KV_HDR <= KV_SIZE) {
    if (ee_get(off) != KV_MAGIC)
      break;
    off = (uint16_t)(off + KV_HDR + ee_get(off + 2u));
  }
  return off;
}

/** @brief XOR checksum of a value buffer (matches the record @c crc field). */
static uint8_t kv_crc(const uint8_t *value, uint8_t size) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < size; i++)
    crc ^= value[i];
  return crc;
}

hal_status_t hal_flash_save(uint8_t key, const uint8_t *value, uint8_t size) {
  if (value == NULL || size == 0u)
    return HAL_ERR_INVALID_ARG;

  /* Supersede any existing live record for this key. */
  uint16_t old = kv_find(key, KV_VALID);
  if (old != 0xFFFFu)
    ee_put(old + 3u, KV_DELETED);

  uint16_t off = kv_end();
  if ((uint32_t)off + KV_HDR + size > KV_SIZE)
    return HAL_ERR; /* full — caller should erase / compact. */

  ee_put(off + 0u, KV_MAGIC);
  ee_put(off + 1u, key);
  ee_put(off + 2u, size);
  ee_put(off + 3u, KV_VALID);
  ee_put(off + 4u, 0u); /* reserved. */
  ee_put(off + 5u, kv_crc(value, size));
  for (uint8_t i = 0; i < size; i++)
    ee_put((uint16_t)(off + KV_HDR + i), value[i]);
  return HAL_OK;
}

hal_status_t hal_flash_read(uint8_t key, uint8_t *value, uint8_t *size) {
  if (value == NULL || size == NULL)
    return HAL_ERR_INVALID_ARG;

  uint16_t off = kv_find(key, KV_VALID);
  if (off == 0xFFFFu)
    return HAL_ERR; /* key not found. */

  uint8_t rsize = ee_get(off + 2u);
  uint8_t rcrc = ee_get(off + 5u);
  for (uint8_t i = 0; i < rsize; i++)
    value[i] = ee_get((uint16_t)(off + KV_HDR + i));
  *size = rsize;
  return (kv_crc(value, rsize) == rcrc) ? HAL_OK : HAL_ERR;
}

hal_status_t hal_flash_delete(uint8_t key) {
  uint16_t off = kv_find(key, KV_VALID);
  if (off == 0xFFFFu)
    return HAL_ERR;
  ee_put(off + 3u, KV_DELETED);
  return HAL_OK;
}

hal_status_t hal_flash_erase(void) {
  for (uint16_t off = 0; off < KV_SIZE; off++)
    ee_put(off, 0xFFu);
  return HAL_OK;
}

bool hal_flash_needs_compaction(void) {
  /* True once too little contiguous free space remains to be useful. */
  return (uint32_t)kv_end() + KV_HDR + 16u >= KV_SIZE;
}
