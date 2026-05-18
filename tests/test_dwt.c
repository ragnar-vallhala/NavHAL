#include "test_dwt.h"
#include "core/cortex-m4/dwt.h"
#include "core/cortex-m4/dwt_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

void test_dwt_init_enables_counters(void) {
  hal_cycle_counter_init();

  // Verify TRCENA bit in DEMCR
  TEST_ASSERT_BITS_HIGH(CORE_DEBUG_DEMCR_TRCENA_BIT, CoreDebug->DEMCR);

  // Verify CYCCNTENA bit in DWT_CTRL
  TEST_ASSERT_BITS_HIGH(DWT_CTRL_CYCCNTENA_BIT, DWT->CTRL);
}

void test_dwt_get_cycles_increments(void) {
  hal_cycle_counter_init();
  uint32_t c1 = hal_cycle_counter_get();
  // Small busy wait or just NOPs
  for (volatile int i = 0; i < 100; i++)
    __asm__ volatile("nop");
  uint32_t c2 = hal_cycle_counter_get();

  TEST_ASSERT_TRUE(c2 > c1);
}

void test_dwt_reset_cycles_zeros_counter(void) {
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
  hal_cycle_counter_init();
  uint32_t delay = 1000;
  uint32_t start = hal_cycle_counter_get();
  hal_cycle_counter_delay(delay);
  uint32_t end = hal_cycle_counter_get();

  TEST_ASSERT_TRUE((end - start) >= delay);
}
