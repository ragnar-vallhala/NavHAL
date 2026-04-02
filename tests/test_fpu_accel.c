#include "test_fpu_accel.h"
#include "core/cortex-m4/dwt.h"
#include "navtest/navtest.h"
#include <stdint.h>

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
  uint32_t start, end;
  const int iterations = 1000;

  dwt_init();
  dwt_reset_cycles();

  start = dwt_get_cycles();
  for (int i = 0; i < iterations; i++) {
    f3 = f1 * f2 + f1;
  }
  end = dwt_get_cycles();

  uint32_t total_cycles = end - start;

  // A typical FADD/FMUL takes 1 cycle.
  // With overhead (loop, volatile load/store), we expect ~10-20 cycles per
  // iteration. Without FPU (soft-float), it would be hundreds of cycles per
  // iteration. So 1000 iterations should take < 50,000 cycles.
  TEST_ASSERT_TRUE(total_cycles < 50000);
  TEST_ASSERT_TRUE(total_cycles > 0);
}
