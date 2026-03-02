/**
 * @file navtest.h
 * @brief NavTest - Lightweight unit test framework for bare-metal ARM Cortex-M4
 *
 * Drop-in replacement for Unity. Output via uart2_write (no libc printf).
 * Uses setjmp/longjmp for per-test abort on failure — same approach as Unity.
 *
 * Global state is defined in navtest_state.c (compiled once).
 * All other files just include this header.
 *
 * Usage:
 *   NAVTEST_BEGIN();
 *   RUN_TEST(my_test_fn);
 *   int failures = NAVTEST_END();
 */

#ifndef NAVTEST_H
#define NAVTEST_H

#include "core/cortex-m4/uart.h"
#include <setjmp.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * ANSI color escapes  (define NAVTEST_NO_COLOR before including to disable)
 * ---------------------------------------------------------------------- */
#ifndef NAVTEST_NO_COLOR
#  define _NT_RST    "\033[0m"
#  define _NT_BOLD   "\033[1m"
#  define _NT_RED    "\033[31m"
#  define _NT_GREEN  "\033[32m"
#  define _NT_YELLOW "\033[33m"
#  define _NT_CYAN   "\033[36m"
#else
#  define _NT_RST    ""
#  define _NT_BOLD   ""
#  define _NT_RED    ""
#  define _NT_GREEN  ""
#  define _NT_YELLOW ""
#  define _NT_CYAN   ""
#endif


/* -------------------------------------------------------------------------
 * Internal state (defined once in navtest_state.c)
 * ---------------------------------------------------------------------- */

typedef struct {
  uint32_t tests;    /**< Tests run in current group */
  uint32_t failures; /**< Failures in current group */
  uint32_t passes;   /**< Passes in current group */
} _NavTestState;

extern _NavTestState _navtest;
extern jmp_buf _navtest_jmp;

/* -------------------------------------------------------------------------
 * setUp / tearDown — defined as weak no-ops in navtest_state.c
 * Test files may define their own versions to override.
 * ---------------------------------------------------------------------- */
void setUp(void);
void tearDown(void);

/* -------------------------------------------------------------------------
 * Internal helpers (static inline — safe to include in multiple TUs)
 * ---------------------------------------------------------------------- */

static inline void _navtest_print_uint32(uint32_t v) {
  char buf[12];
  int i = 11;
  buf[i] = '\0';
  if (v == 0) {
    buf[--i] = '0';
  }
  while (v && i > 0) {
    buf[--i] = '0' + (v % 10);
    v /= 10;
  }
  uart2_write(buf + i);
}

static inline void _navtest_fail(const char *file, uint32_t line,
                                 const char *msg) {
  uart2_write(_NT_RED "  FAIL: " _NT_RST);
  uart2_write(file);
  uart2_write(":");
  _navtest_print_uint32(line);
  uart2_write(" -- ");
  uart2_write(msg);
  uart2_write("\r\n");
  _navtest.failures++;
  longjmp(_navtest_jmp, 1);
}

static inline int _navtest_end_impl(void) {
  uart2_write("  [");
  _navtest_print_uint32(_navtest.tests);
  uart2_write(" tests | ");
  _navtest_print_uint32(_navtest.failures);
  uart2_write(" failed | ");
  _navtest_print_uint32(_navtest.passes);
  uart2_write(" passed]\r\n");
  return (int)_navtest.failures;
}

/* -------------------------------------------------------------------------
 * Framework API
 * ---------------------------------------------------------------------- */

/** Begin a test group — resets per-group counters. */
#define NAVTEST_BEGIN()                                                        \
  do {                                                                         \
    _navtest.tests = 0;                                                        \
    _navtest.failures = 0;                                                     \
    _navtest.passes = 0;                                                       \
  } while (0)

/** End a test group — prints summary. Returns number of failures. */
#define NAVTEST_END() _navtest_end_impl()

/** Returns the number of tests run in the current group. */
static inline uint32_t navtest_get_test_count(void) { return _navtest.tests; }

/**
 * Run a single test function, calling setUp/tearDown around it.
 * Failures inside the function abort via longjmp without crashing the runner.
 */
#define RUN_TEST(fn)                                                           \
  do {                                                                         \
    _navtest.tests++;                                                          \
    setUp();                                                                   \
    uart2_write(_NT_CYAN "  >> " _NT_BOLD #fn _NT_RST "\r\n");               \
    if (setjmp(_navtest_jmp) == 0) {                                           \
      fn();                                                                    \
      _navtest.passes++;                                                       \
      uart2_write(_NT_BOLD _NT_GREEN "  PASS" _NT_RST "\r\n");               \
    }                                                                          \
    tearDown();                                                                \
  } while (0)

/* -------------------------------------------------------------------------
 * Assertion macros
 * ---------------------------------------------------------------------- */

#define TEST_ASSERT_EQUAL_UINT32(expected, actual)                             \
  do {                                                                         \
    uint32_t _e = (uint32_t)(expected);                                        \
    uint32_t _a = (uint32_t)(actual);                                          \
    if (_e != _a) {                                                            \
      uart2_write("  Expected: ");                                             \
      _navtest_print_uint32(_e);                                               \
      uart2_write("  Got: ");                                                  \
      _navtest_print_uint32(_a);                                               \
      uart2_write("\r\n");                                                     \
      _navtest_fail(__FILE__, __LINE__, "TEST_ASSERT_EQUAL_UINT32");           \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                              \
  do {                                                                         \
    if ((void *)(ptr) == (void *)0)                                            \
      _navtest_fail(__FILE__, __LINE__,                                        \
                    "TEST_ASSERT_NOT_NULL: pointer is NULL");                  \
  } while (0)

#define TEST_ASSERT_TRUE(cond)                                                 \
  do {                                                                         \
    if (!(cond))                                                               \
      _navtest_fail(__FILE__, __LINE__,                                        \
                    "TEST_ASSERT_TRUE: condition is false");                   \
  } while (0)

#define TEST_ASSERT_FALSE(cond)                                                \
  do {                                                                         \
    if ((cond))                                                                \
      _navtest_fail(__FILE__, __LINE__,                                        \
                    "TEST_ASSERT_FALSE: condition is true");                   \
  } while (0)

/** Assert that all bits set in @p mask are also set in @p val */
#define TEST_ASSERT_BITS_HIGH(mask, val)                                       \
  do {                                                                         \
    uint32_t _m = (uint32_t)(mask);                                            \
    uint32_t _v = (uint32_t)(val);                                             \
    if ((_v & _m) != _m) {                                                     \
      uart2_write("  Mask: ");                                                 \
      _navtest_print_uint32(_m);                                               \
      uart2_write("  Val:  ");                                                 \
      _navtest_print_uint32(_v);                                               \
      uart2_write("\r\n");                                                     \
      _navtest_fail(__FILE__, __LINE__,                                        \
                    "TEST_ASSERT_BITS_HIGH: bits not set");                    \
    }                                                                          \
  } while (0)

/** Assert that all bits set in @p mask are cleared in @p val */
#define TEST_ASSERT_BITS_LOW(mask, val)                                        \
  do {                                                                         \
    uint32_t _m = (uint32_t)(mask);                                            \
    uint32_t _v = (uint32_t)(val);                                             \
    if ((_v & _m) != 0) {                                                      \
      uart2_write("  Mask: ");                                                 \
      _navtest_print_uint32(_m);                                               \
      uart2_write("  Val:  ");                                                 \
      _navtest_print_uint32(_v);                                               \
      uart2_write("\r\n");                                                     \
      _navtest_fail(__FILE__, __LINE__,                                        \
                    "TEST_ASSERT_BITS_LOW: bits not cleared");                 \
    }                                                                          \
  } while (0)

#endif /* NAVTEST_H */
