#ifndef TEST_FPU_ACCEL_H
#define TEST_FPU_ACCEL_H

#include "common/hal_features.h"
#include "navtest/navtest.h"

#if NAVHAL_HAS_FPU

void test_fpu_basic_arithmetic(void);
void test_fpu_benchmark_cycles(void);
void test_hal_fpu_enable_returns_ok(void);

extern const navtest_suite_t test_fpu_suite;

#endif /* NAVHAL_HAS_FPU */
#endif // TEST_FPU_ACCEL_H
