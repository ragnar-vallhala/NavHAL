#ifndef TEST_FPU_ACCEL_H
#define TEST_FPU_ACCEL_H

#include "navtest/navtest.h"

void test_fpu_basic_arithmetic(void);
void test_fpu_benchmark_cycles(void);

extern const navtest_suite_t test_fpu_suite;

#endif // TEST_FPU_ACCEL_H
