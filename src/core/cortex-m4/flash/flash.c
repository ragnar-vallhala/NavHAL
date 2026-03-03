#include "core/cortex-m4/flash.h"
#include "common/hal_types.h"
#include "core/cortex-m4/flash_reg.h"
#include "core/cortex-m4/uart.h"
#include "utils/util.h"
#include <stddef.h>
#include <stdint.h>

// Interal function signatures
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

void _flash_erase_sector_(byte sector) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_SNB_Msk;
  FLASH_CR |= FLASH_CR_SER | ((sector & 0xF) << FLASH_CR_SNB_Pos);
  FLASH_CR |= FLASH_CR_STRT;
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_SER;
  _flash_lock_();
}

void _flash_program_word_(uint32_t addr, uint32_t data) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
  FLASH_CR |= (0x2U << FLASH_CR_PSIZE_Pos); // x32 programming
  FLASH_CR |= FLASH_CR_PG;

  *(volatile uint32_t *)addr = data;

  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PG;
  _flash_lock_();
}

void _flash_program_half_word_(uint32_t addr, uint16_t data) {
  _flash_unlock_();
  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
  FLASH_CR |= (0x1U << FLASH_CR_PSIZE_Pos); // x16 programming
  FLASH_CR |= FLASH_CR_PG;

  *(volatile uint16_t *)addr = data;

  _flash_wait_();
  FLASH_CR &= ~FLASH_CR_PG;
  _flash_lock_();
}

uint32_t _flash_read_word_(uint32_t addr) { return *(volatile uint32_t *)addr; }

uint16_t _flash_read_half_word_(uint32_t addr) {
  return *(volatile uint16_t *)addr;
}
static byte _flash_calculate_crc_(const byte *value, byte size) {
  if (size == 0)
    return 0;
  byte crc = value[0];
  for (int i = 1; i < size; i++) {
    crc ^= value[i];
  }
  return crc;
}
__IO byte *_flash_find_next_free(void) {
  __IO byte *ptr = (__IO byte *)FLASH_PRIMARY_STORAGE_START;
  FlashRecord_t *rec = (FlashRecord_t *)ptr;
  while (rec->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr < FLASH_PRIMARY_STORAGE_END) {
    ptr = ptr + rec->size + sizeof(FlashRecord_t);
    rec = (FlashRecord_t *)ptr;
  }
  if ((uint32_t)ptr >= FLASH_PRIMARY_STORAGE_END)
    return NULL;
  return ptr;
}

__IO FlashRecord_t *_flash_find_first_valid_entry_(byte key) {

  __IO FlashRecord_t *rec = (__IO FlashRecord_t *)FLASH_PRIMARY_STORAGE_START;
  while ((uint32_t)rec < FLASH_PRIMARY_STORAGE_END &&
         rec->magic == FLASH_MAGIC_NUMBER) {
    if (rec->key == key && rec->status == FLASH_VALID)
      return rec;
    byte *byte_addr = (byte *)rec;
    byte_addr = byte_addr + rec->size + sizeof(FlashRecord_t);
    rec = (FlashRecord_t *)byte_addr;
  }
  return NULL;
}
FlashStatus_t _flash_write_data_(uint32_t addr, const byte *data, byte size) {
  if (size == 0)
    return FLASH_ERR_WRITE;

  byte padded_size = (size % 2 == 0) ? size : size + 1;
  uint16_t half_word = 0;

  for (byte i = 0; i < padded_size; i += 2) {
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

  return FLASH_OK;
}

FlashStatus_t _flash_read_data_(uint32_t addr, byte *data, byte size) {
  if (size == 0)
    return FLASH_ERR_NOT_FOUND;

  byte padded_size = (size % 2 == 0) ? size : size + 1;
  uint16_t half_word = 0;
  for (byte i = 0; i < padded_size; i += 2) {
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
  return FLASH_OK;
}

FlashStatus_t _flash_shift_sector_primary_to_secondary_(void) {
  byte *ptr_primary = (byte *)FLASH_PRIMARY_STORAGE_START;
  byte *ptr_secondary = (byte *)FLASH_SECONDARY_STORAGE_START;
  FlashRecord_t *rec_primary = (FlashRecord_t *)ptr_primary;
  while (rec_primary->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr_primary < FLASH_PRIMARY_STORAGE_END) {
    if (rec_primary->status != FLASH_VALID) {
      byte total_size = sizeof(FlashRecord_t) + rec_primary->size;
      ptr_primary += total_size;
      rec_primary = (FlashRecord_t *)ptr_primary;
      continue;
    }
    byte total_size = sizeof(FlashRecord_t) + rec_primary->size;
    FlashStatus_t status =
        _flash_write_data_((uint32_t)ptr_secondary, ptr_primary, total_size);
    if (status != FLASH_OK)
      return status;
    ptr_primary += total_size;
    ptr_secondary += total_size;
    rec_primary = (FlashRecord_t *)ptr_primary;
  }

  return FLASH_OK;
}

FlashStatus_t _flash_shift_sector_secondary_to_primary_(void) {
  byte *ptr_primary = (byte *)FLASH_PRIMARY_STORAGE_START;
  byte *ptr_secondary = (byte *)FLASH_SECONDARY_STORAGE_START;
  FlashRecord_t *rec_secondary = (FlashRecord_t *)ptr_secondary;
  while (rec_secondary->magic == FLASH_MAGIC_NUMBER &&
         (uint32_t)ptr_secondary < FLASH_SECONDARY_STORAGE_END) {
    byte total_size = sizeof(FlashRecord_t) + rec_secondary->size;
    FlashStatus_t status =
        _flash_write_data_((uint32_t)ptr_primary, ptr_secondary, total_size);
    if (status != FLASH_OK)
      return status;
    ptr_primary += total_size;
    ptr_secondary += total_size;
    rec_secondary = (FlashRecord_t *)ptr_secondary;
  }
  return FLASH_OK;
}

FlashStatus_t _flash_compact_storage_(void) {
  uart2_write("Compacting flash storage...\n");
  FlashStatus_t status;
  status = _flash_shift_sector_primary_to_secondary_();
  if (status != FLASH_OK)
    return status;
  uart2_write("Erasing primary sector...\n");
  _flash_erase_sector_(PRIMARY_FLASH_SECTOR);
  status = _flash_shift_sector_secondary_to_primary_();
  if (status != FLASH_OK)
    return status;
  uart2_write("Erasing secondary sector...\n");
  _flash_erase_sector_(SECONDARY_FLASH_SECTOR);
  return FLASH_OK;
}

FlashStatus_t save_data_to_flash(byte key, const byte *value, byte size) {
  if (size == 0)
    return FLASH_ERR_NOT_FOUND;

  __IO byte *ptr = _flash_find_next_free();
  if (ptr == NULL) {
    FlashStatus_t status = _flash_compact_storage_();
    if (status != FLASH_OK)
      return status;
    ptr = _flash_find_next_free();
    if (ptr == NULL)
      return FLASH_ERR_WRITE;
  }

  __IO FlashRecord_t *last_rec = _flash_find_first_valid_entry_(key);
  if (last_rec != NULL) {
    FlashRecord_t updated_last_rec;
    hal_memcpy(&updated_last_rec, (const void *)last_rec,
               sizeof(FlashRecord_t));
    updated_last_rec.status = FLASH_DELETED;
    FlashStatus_t status = _flash_write_data_((uint32_t)(last_rec),
                                              (const byte *)&updated_last_rec,
                                              sizeof(FlashRecord_t));
    if (status != FLASH_OK)
      return status;
  }

  FlashRecord_t rec;
  rec.key = key;
  rec.magic = FLASH_MAGIC_NUMBER;
  rec.size = size;
  rec.status = FLASH_VALID;
  rec.crc = _flash_calculate_crc_(value, size);
  FlashStatus_t status = _flash_write_data_((uint32_t)ptr, (const byte *)&rec,
                                            sizeof(FlashRecord_t));
  if (status != FLASH_OK)
    return status;
  ptr += (sizeof(FlashRecord_t) % 2 == 0 ? sizeof(FlashRecord_t)
                                         : sizeof(FlashRecord_t) + 1);
  status = _flash_write_data_((uint32_t)ptr, value, size);
  return status;
}

FlashStatus_t read_data_from_flash(byte key, byte *value, byte *size) {
  __IO FlashRecord_t *last_rec = _flash_find_first_valid_entry_(key);
  if (last_rec == NULL) {
    *size = 0;
    return FLASH_ERR_NOT_FOUND;
  }
  byte *src = ((byte *)last_rec) + sizeof(FlashRecord_t);
  *size = last_rec->size;
  hal_memcpy(value, src, *size);
  if (last_rec->crc != _flash_calculate_crc_(value, *size))
    return FLASH_ERR_NOT_FOUND;
  return FLASH_OK;
}
