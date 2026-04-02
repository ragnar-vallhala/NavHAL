#include "test_dwt.h"
#include "core/cortex-m4/dwt.h"
#include "core/cortex-m4/dwt_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

void test_dwt_init_enables_counters(void) {
  dwt_init();

  // Verify TRCENA bit in DEMCR
  TEST_ASSERT_BITS_HIGH(CORE_DEBUG_DEMCR_TRCENA_BIT, CoreDebug->DEMCR);

  // Verify CYCCNTENA bit in DWT_CTRL
  TEST_ASSERT_BITS_HIGH(DWT_CTRL_CYCCNTENA_BIT, DWT->CTRL);
}

void test_dwt_get_cycles_increments(void) {
  dwt_init();
  uint32_t c1 = dwt_get_cycles();
  // Small busy wait or just NOPs
  for (volatile int i = 0; i < 100; i++)
    __asm__ volatile("nop");
  uint32_t c2 = dwt_get_cycles();

  TEST_ASSERT_TRUE(c2 > c1);
}

void test_dwt_reset_cycles_zeros_counter(void) {
  dwt_init();
  // Let it run a bit
  for (volatile int i = 0; i < 100; i++)
    __asm__ volatile("nop");

  TEST_ASSERT_TRUE(dwt_get_cycles() > 0);

  dwt_reset_cycles();
  // Allow a small number of cycles to elapse during function call/read
  TEST_ASSERT_TRUE(dwt_get_cycles() < 50);
}

void test_dwt_delay_cycles_elapses_time(void) {
  dwt_init();
  uint32_t delay = 1000;
  uint32_t start = dwt_get_cycles();
  dwt_delay_cycles(delay);
  uint32_t end = dwt_get_cycles();

  TEST_ASSERT_TRUE((end - start) >= delay);
}
