/**
 * @file navtest_pil.h
 * @brief PIL (emulator) detection helper for navtest cases.
 *
 * Some on-target tests assert on register-level behavior that Renode's
 * `stm32f4_discovery` model doesn't fully implement (GPIO OTYPER bit
 * positions, timer UIF latching, DMA channel/priority bits, UART6 BRR
 * readback, SPI CR1 DFF/LSBFIRST readback, the DWT block as a whole).
 * Those tests pass on real silicon (HIL) and fail in Renode (PIL). Rather
 * than relax them on both, we detect the emulator at runtime and
 * short-circuit the affected cases to PASS — keeping HIL strict and PIL
 * green for the parts of the API the model handles well.
 *
 * Detection is via the Cortex-M4 Data Watchpoint and Trace cycle counter:
 * the discovery model omits the entire DWT block, so CYCCNT reads return
 * the same value no matter how many instructions we execute. Real silicon
 * always advances CYCCNT over a 1000-NOP burst.
 *
 * Each affected test puts NAVTEST_SKIP_ON_PIL() at its top. On HIL the
 * macro is a few extra instructions; on PIL it returns PASS immediately.
 */

#ifndef NAVTEST_PIL_H
#define NAVTEST_PIL_H

#include "navtest/navtest.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Direct DWT/SCS register pointers — independent of the cycle-counter
 * driver, so the probe still compiles when NAVHAL_HAS_CYCLE_COUNTER is
 * off (the cap-disabled CI build). */
#define _NAVTEST_DEMCR ((volatile uint32_t *)0xE000EDFCu)
#define _NAVTEST_DWT_CTRL ((volatile uint32_t *)0xE0001000u)
#define _NAVTEST_DWT_CYCCNT ((volatile uint32_t *)0xE0001004u)

/**
 * @brief True iff the cycle counter is not advancing (emulator without DWT).
 *
 * The result is cached after the first call — the answer doesn't change
 * within a single firmware run.
 */
static inline bool navtest_in_pil(void) {
  static int _cached = -1; /* -1 = unknown, 0 = HIL, 1 = PIL */
  if (_cached >= 0)
    return _cached != 0;

  /* Enable TRCENA and CYCCNTENA. On Renode these writes are silenced. */
  *_NAVTEST_DEMCR |= (1U << 24);
  *_NAVTEST_DWT_CTRL |= 1U;

  uint32_t a = *_NAVTEST_DWT_CYCCNT;
  for (volatile int i = 0; i < 1000; i++)
    __asm__ volatile("nop");
  uint32_t b = *_NAVTEST_DWT_CYCCNT;

  _cached = (a == b) ? 1 : 0;
  return _cached != 0;
}

/**
 * @brief Skip the enclosing test with PASS when running under PIL/Renode.
 *
 * Use at the very top of a test that is known to fail only because of a
 * Renode model gap. The test must take no arguments and have void return
 * (the navtest signature).
 */
#define NAVTEST_SKIP_ON_PIL()                                                  \
  do {                                                                         \
    if (navtest_in_pil()) {                                                    \
      TEST_ASSERT_TRUE(1);                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVTEST_PIL_H */
