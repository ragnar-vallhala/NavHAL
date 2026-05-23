#ifndef TEST_HOST_HAL_STATUS_H
#define TEST_HOST_HAL_STATUS_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_hal_status_ok_is_zero(void);
void test_hal_status_err_is_one(void);
void test_hal_status_distinct_codes(void);
void test_hal_ok_or_return_passes_through(void);
void test_hal_ok_or_return_short_circuits(void);

extern const navtest_suite_t test_hal_status_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
