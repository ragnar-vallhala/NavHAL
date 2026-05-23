#define CORTEX_M4
#include "test_dwt.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdint.h>

#if NAVHAL_HAS_CYCLE_COUNTER
#include "navhal_port_dwt.h"
#include "family/dwt_reg.h"

void test_dwt_init_enables_counters(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_cycle_counter_init();

  // Verify TRCENA bit in DEMCR
  TEST_ASSERT_BITS_HIGH(CORE_DEBUG_DEMCR_TRCENA_BIT, CoreDebug->DEMCR);

  // Verify CYCCNTENA bit in DWT_CTRL
  TEST_ASSERT_BITS_HIGH(DWT_CTRL_CYCCNTENA_BIT, DWT->CTRL);
}

void test_dwt_get_cycles_increments(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_cycle_counter_init();
  uint32_t c1 = hal_cycle_counter_get();
  // Small busy wait or just NOPs
  for (volatile int i = 0; i < 100; i++)
    __asm__ volatile("nop");
  uint32_t c2 = hal_cycle_counter_get();

  TEST_ASSERT_TRUE(c2 > c1);
}

void test_dwt_reset_cycles_zeros_counter(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_cycle_counter_init();
  // Let it run a bit
  for (volatile int i = 0; i < 100; i++)
    __asm__ volatile("nop");

  TEST_ASSERT_TRUE(hal_cycle_counter_get() > 0);

  hal_cycle_counter_reset();
  // Allow a small number of cycles to elapse during function call/read
  TEST_ASSERT_TRUE(hal_cycle_counter_get() < 50);
}

void test_dwt_delay_cycles_elapses_time(void) {
  /* hal_cycle_counter_delay() busy-waits on CYCCNT. Without the skip
   * the suite hangs forever in PIL where CYCCNT never advances. */
  NAVTEST_SKIP_ON_PIL();
  hal_cycle_counter_init();
  uint32_t delay = 1000;
  uint32_t start = hal_cycle_counter_get();
  hal_cycle_counter_delay(delay);
  uint32_t end = hal_cycle_counter_get();

  TEST_ASSERT_TRUE((end - start) >= delay);
}

/* -------------------- Standardized contract -------------------- */

void test_hal_cycle_counter_init_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_cycle_counter_init());
}

void test_hal_cycle_counter_reset_returns_ok(void) {
  hal_cycle_counter_init();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_cycle_counter_reset());
}

static const navtest_case_t dwt_cases[] = {
    NAVTEST_CASE(test_dwt_init_enables_counters),
    NAVTEST_CASE(test_dwt_get_cycles_increments),
    NAVTEST_CASE(test_dwt_reset_cycles_zeros_counter),
    NAVTEST_CASE(test_dwt_delay_cycles_elapses_time),
    NAVTEST_CASE(test_hal_cycle_counter_init_returns_ok),
    NAVTEST_CASE(test_hal_cycle_counter_reset_returns_ok),
};

const navtest_suite_t test_dwt_suite = {
    .name = "CYCLE_COUNTER",
    .cases = dwt_cases,
    .count = sizeof(dwt_cases) / sizeof(dwt_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_HAS_CYCLE_COUNTER */
