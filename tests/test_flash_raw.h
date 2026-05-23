#ifndef TEST_FLASH_RAW_H
#define TEST_FLASH_RAW_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_flash_storage_integration(void);
void test_hal_flash_save_rejects_null_value(void);
void test_hal_flash_read_rejects_null_pointers(void);
void test_hal_flash_delete_then_read_returns_error(void);
void test_hal_flash_needs_compaction_returns_bool(void);
void test_hal_flash_erase_returns_ok(void);

extern const navtest_suite_t test_flash_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_FLASH_RAW_H
