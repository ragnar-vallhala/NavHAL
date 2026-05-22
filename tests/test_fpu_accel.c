#define CORTEX_M4
#include "test_fpu_accel.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdint.h>

#if NAVHAL_HAS_FPU
#include "navhal_port_dwt.h"
#include "navhal_port_fpu.h"

static volatile float f1 = 1.23456f;
static volatile float f2 = 2.34567f;
static volatile float f3;

void test_fpu_basic_arithmetic(void) {
  f3 = f1 + f2;
  TEST_ASSERT_TRUE(f3 > 3.0f && f3 < 4.0f);
  f3 = f1 * f2;
  TEST_ASSERT_TRUE(f3 > 2.0f && f3 < 3.0f);
  f3 = f1 / f2;
  TEST_ASSERT_TRUE(f3 > 0.0f && f3 < 1.0f);
}

void test_fpu_benchmark_cycles(void) {
  /* Depends on DWT cycles advancing; PIL silences the DWT range. */
  NAVTEST_SKIP_ON_PIL();
  uint32_t start, end;
  const int iterations = 1000;

  hal_cycle_counter_init();
  hal_cycle_counter_reset();

  start = hal_cycle_counter_get();
  for (int i = 0; i < iterations; i++) {
    f3 = f1 * f2 + f1;
  }
  end = hal_cycle_counter_get();

  uint32_t total_cycles = end - start;

  // A typical FADD/FMUL takes 1 cycle.
  // With overhead (loop, volatile load/store), we expect ~10-20 cycles per
  // iteration. Without FPU (soft-float), it would be hundreds of cycles per
  // iteration. So 1000 iterations should take < 50,000 cycles.
  TEST_ASSERT_TRUE(total_cycles < 50000);
  TEST_ASSERT_TRUE(total_cycles > 0);
}

void test_hal_fpu_enable_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_fpu_enable());
}

static const navtest_case_t fpu_cases[] = {
    NAVTEST_CASE(test_fpu_basic_arithmetic),
    NAVTEST_CASE(test_fpu_benchmark_cycles),
    NAVTEST_CASE(test_hal_fpu_enable_returns_ok),
};

const navtest_suite_t test_fpu_suite = {
    .name = "FPU",
    .cases = fpu_cases,
    .count = sizeof(fpu_cases) / sizeof(fpu_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_HAS_FPU */
