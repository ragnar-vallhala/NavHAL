#include "core/cortex-m4/flash.h"
#include "core/cortex-m4/flash_reg.h"
#include <stdint.h>
#include "utils/util.h"
#include <stddef.h>
#include "core/cortex-m4/uart.h"

// Interal function signatures
static void _flash_wait_(void)
{
    while (FLASH_SR & FLASH_SR_BSY)
        ;
}

static void _flash_unlock_(void)
{
    if (FLASH_CR & FLASH_CR_LOCK)
    {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

static void _flash_lock_(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

void _flash_erase_sector_(uint8_t sector)
{
    _flash_unlock_();
    _flash_wait_();
    FLASH_CR &= ~FLASH_CR_SNB_Msk;
    FLASH_CR |= FLASH_CR_SER | ((sector & 0xF) << FLASH_CR_SNB_Pos);
    FLASH_CR |= FLASH_CR_STRT;
    _flash_wait_();
    FLASH_CR &= ~FLASH_CR_SER;
    _flash_lock_();
}

void _flash_program_word_(uint32_t addr, uint32_t data)
{
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

void _flash_program_half_word_(uint32_t addr, uint16_t data)
{
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

uint32_t _flash_read_word_(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

uint16_t _flash_read_half_word_(uint32_t addr)
{
    return *(volatile uint16_t *)addr;
}
static uint8_t _flash_calculate_crc_(const uint8_t *value, uint8_t size)
{
    if (size == 0)
        return 0;
    uint8_t crc = value[0];
    for (int i = 1; i < size; i++)
    {
        crc ^= value[i];
    }
    return crc;
}
uint8_t *_flash_find_next_free(void)
{
    uint8_t *ptr = (volatile uint8_t *)FLASH_STORAGE_START;
    FlashRecord_t *rec = (FlashRecord_t *)ptr;
    while (rec->magic == FLASH_MAGIC_NUMBER && ptr < FLASH_STORAGE_END)
    {
        ptr = ptr + rec->size + sizeof(FlashRecord_t);
        rec = (FlashRecord_t *)ptr;
    }
    if (ptr >= FLASH_STORAGE_END)
        return NULL;
    return ptr;
}

FlashRecord_t *_flash_find_first_valid_entry_(uint8_t key)
{

    FlashRecord_t *rec = (volatile FlashRecord_t *)FLASH_STORAGE_START;
    int i = 0;
    while (rec < FLASH_STORAGE_END && rec->magic == FLASH_MAGIC_NUMBER)
    {
        if (rec->key == key && rec->status == FLASH_VALID)
            return rec;
        uint8_t *byte_addr = (uint8_t *)rec;
        byte_addr = byte_addr + rec->size + sizeof(FlashRecord_t);
        rec = (FlashRecord_t *)byte_addr;
    }
    return NULL;
}
FlashStatus_t _flash_write_data_(uint32_t addr, uint8_t *data, uint8_t size)
{
    if (size == 0)
        return FLASH_ERR_WRITE;

    uint8_t padded_size = (size % 2 == 0) ? size : size + 1;
    uint16_t half_word = 0;

    for (uint8_t i = 0; i < padded_size; i += 2)
    {
        if (i + 1 < size)
        {
            // Normal case: 2 valid bytes
            half_word = (data[i + 1] << 8) | data[i];
        }
        else
        {
            // Last odd byte — pad with 0xFF (or 0x00 if you prefer)
            half_word = (0xFF << 8) | data[i];
        }
        _flash_program_half_word_(addr, half_word);
        addr += 2;
    }

    return FLASH_OK;
}

FlashStatus_t _flash_read_data_(uint32_t addr, uint8_t *data, uint8_t size)
{
    if (size == 0)
        return FLASH_ERR_NOT_FOUND;

    uint8_t padded_size = (size % 2 == 0) ? size : size + 1;
    uint16_t half_word = 0;
    for (uint8_t i = 0; i < padded_size; i += 2)
    {
        half_word = _flash_read_half_word_(addr);
        if (i + 1 < size)
        {
            // Normal case: 2 valid bytes
            data[i] = half_word & (0xFF);
            data[i + 1] = (half_word >> 8) & (0xFF);
        }
        else
        {
            data[i] = half_word & (0xFF);
        }

        addr += 2;
    }
    return FLASH_OK;
}

FlashStatus_t save_data_to_flash(uint8_t key, const uint8_t *value, uint8_t size)
{
    uint8_t *ptr = _flash_find_next_free();
    if (ptr == NULL || size == 0)
        return FLASH_ERR_NOT_FOUND;
    FlashRecord_t *last_rec = _flash_find_first_valid_entry_(key);
    if (last_rec != NULL)
    {
        FlashRecord_t updated_last_rec;
        hal_memcpy(&updated_last_rec, last_rec, sizeof(FlashRecord_t));
        updated_last_rec.status = FLASH_DELETED;
        FlashStatus_t status = _flash_write_data_((uint32_t)(last_rec),
                                                  &updated_last_rec, sizeof(FlashRecord_t));
        if (status != FLASH_OK)
            return status;
    }

    FlashRecord_t rec;
    rec.key = key;
    rec.magic = FLASH_MAGIC_NUMBER;
    rec.size = size;
    rec.status = FLASH_VALID;
    rec.crc = _flash_calculate_crc_(value, size);
    FlashStatus_t status = _flash_write_data_((uint32_t)ptr, &rec, sizeof(FlashRecord_t));
    if (status != FLASH_OK)
        return status;
    ptr += (sizeof(FlashRecord_t) % 2 == 0 ? sizeof(FlashRecord_t) : sizeof(FlashRecord_t) + 1);
    status = _flash_write_data_((uint32_t)ptr, value, size);
    return status;
}

FlashStatus_t read_data_from_flash(uint8_t key, uint8_t *value, uint8_t *size)
{
    FlashRecord_t *last_rec = _flash_find_first_valid_entry_(key);
    if (last_rec == NULL)
        return FLASH_ERR_NOT_FOUND;
    uint8_t *src = ((uint8_t *)last_rec) + sizeof(FlashRecord_t);
    *size = last_rec->size;
    hal_memcpy(value, src, *size);
    if (last_rec->crc != _flash_calculate_crc_(value, *size))
        return FLASH_ERR_NOT_FOUND;
    return FLASH_OK;
}