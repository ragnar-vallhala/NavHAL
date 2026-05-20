#ifndef TEST_I2C_H
#define TEST_I2C_H

#include "navtest/navtest.h"

void test_i2c_init_config(void);
void test_i2c_fast_mode_config(void);

extern const navtest_suite_t test_i2c_suite;

#endif // TEST_I2C_H
