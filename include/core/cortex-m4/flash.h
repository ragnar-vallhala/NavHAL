#ifndef CORTEX_M4_FLASH_H
#define CORTEX_M4_FLASH_H
#include <stdint.h>


typedef enum
{
    FLASH_OK = 0,
    FLASH_ERR_NOT_FOUND = -1,
    FLASH_ERR_WRITE = -2,
    FLASH_ERR_ERASE = -3
} FlashStatus_t;

typedef struct FlashRecord
{
    uint8_t magic;
    uint8_t key;      // user key
    uint8_t size;     // length of value
    uint8_t status;   // 0xFF = empty, 0x01 = valid, 0x00 = deleted
    uint8_t reserved; // alignment (optional)
    uint8_t crc;      // XOR crc
} FlashRecord_t;

FlashStatus_t save_data_to_flash(uint8_t key, const uint8_t *value, uint8_t size);
FlashStatus_t read_data_from_flash(uint8_t key, uint8_t *value, uint8_t *size);
FlashStatus_t delete_data_from_flash(uint8_t key);
FlashStatus_t flash_storage_erase(void);
int flash_storage_needs_compaction(void);
#endif // CORTEX_M4_FLASH_H
