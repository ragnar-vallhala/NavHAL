#ifndef CORTEX_M4_FLASH_H
#define CORTEX_M4_FLASH_H
#include <stdint.h>
#include "common/hal_types.h"

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERR_NOT_FOUND = -1,
    FLASH_ERR_WRITE = -2,
    FLASH_ERR_ERASE = -3
} FlashStatus_t;

typedef struct FlashRecord
{
    byte magic;
    byte key;      // user key
    byte size;     // length of value
    byte status;   // 0xFF = empty, 0x01 = valid, 0x00 = deleted
    byte reserved; // alignment (optional)
    byte crc;      // XOR crc
} FlashRecord_t;

FlashStatus_t save_data_to_flash(byte key, const byte *value, byte size);
FlashStatus_t read_data_from_flash(byte key, byte *value, byte *size);
FlashStatus_t delete_data_from_flash(byte key);
FlashStatus_t flash_storage_erase(void);
int flash_storage_needs_compaction(void);
#endif // CORTEX_M4_FLASH_H
