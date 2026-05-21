/**
 * @file tests/test_sdio.h
 * @brief Standardized hal_sdio_* API smoke tests.
 *
 * The SDIO driver speaks ::hal_sdio_error_t rather than ::hal_status_t
 * (per its module note); these tests cover the typed-id surface and the
 * error-code contract for the common-failure paths.
 */

#ifndef TEST_SDIO_H
#define TEST_SDIO_H

#include "common/hal_features.h"
#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_SDIO

void test_hal_sdio_init_rejects_null_config(void);
void test_hal_sdio_read_block_rejects_null_buffer(void);
void test_hal_sdio_write_block_rejects_null_buffer(void);
void test_hal_sdio_get_sector_count_returns_value(void);
void test_hal_sdio_set_callback_smoke(void);

extern const navtest_suite_t test_sdio_suite;

#endif /* NAVHAL_HAS_SDIO */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_SDIO_H
