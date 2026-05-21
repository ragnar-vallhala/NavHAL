#ifndef TEST_I2C_H
#define TEST_I2C_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_i2c_init_config(void);
void test_i2c_fast_mode_config(void);
void test_hal_i2c_init_returns_ok(void);
void test_hal_i2c_init_rejects_null_config(void);
void test_hal_i2c_write_rejects_null_data(void);
void test_hal_i2c_read_rejects_null_data(void);
void test_hal_i2c_write_read_rejects_null_data(void);
void test_hal_i2c_typed_id_compiles(void);

extern const navtest_suite_t test_i2c_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_I2C_H
