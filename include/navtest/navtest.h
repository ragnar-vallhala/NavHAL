/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sink for all test output. Backends:
 *   - target (default): writes via the per-port console UART
 *     (NAVTEST_UART, see tests/navtest_target.h)
 *   - host  (-DNAVTEST_HOST): writes to stdout (see tests/host/host_backend.c)
 */
void navtest_write(const char *s);

/* PROGMEM support — assertion source strings and test names land in
 * flash on AVR instead of being copied to .data at startup. ATmega328P
 * has 2 KB SRAM; the conformance suite alone needs ~780 bytes of
 * strings and would push past the limit otherwise.
 *
 * On AVR:
 *   _NT_PSTR(s)          builds a static const char[] in PROGMEM and
 *                        returns its address.
 *   navtest_write_P(p)   reads bytes from PROGMEM one at a time and
 *                        feeds them to the UART (implemented in
 *                        navtest_state.c).
 *
 * On every other arch both collapse to no-ops: _NT_PSTR(s) is just
 * (s), navtest_write_P is #defined to navtest_write. Code below uses
 * the _P form uniformly so the same source compiles on both.
 */
#if defined(__AVR__)
#  include <avr/pgmspace.h>
   /* GCC statement-expression — works on every embedded GCC we target. */
#  define _NT_PSTR(s) (__extension__({                          \
       static const char _navtest_pstr[] __attribute__((__progmem__)) = (s); \
       _navtest_pstr;                                           \
     }))
   void navtest_write_P(const char *progmem_str);
#else
#  define _NT_PSTR(s) (s)
#  define navtest_write_P navtest_write
#endif

/* -------------------------------------------------------------------------
 * ANSI color escapes  (define NAVTEST_NO_COLOR before including to disable)
 * ---------------------------------------------------------------------- */
#ifndef NAVTEST_NO_COLOR
#define _NT_RST "\033[0m"
#define _NT_BOLD "\033[1m"
#define _NT_RED "\033[31m"
#define _NT_GREEN "\033[32m"
#define _NT_YELLOW "\033[33m"
#define _NT_CYAN "\033[36m"
#else
#define _NT_RST ""
#define _NT_BOLD ""
#define _NT_RED ""
#define _NT_GREEN ""
#define _NT_YELLOW ""
#define _NT_CYAN ""
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
  navtest_write(buf + i);
}

/* _navtest_fail receives PROGMEM-tagged pointers on AVR (assertion
 * macros wrap __FILE__ and the assertion message in _NT_PSTR before
 * calling). On non-AVR, navtest_write_P aliases navtest_write so the
 * same source path runs everywhere. */
static inline void _navtest_fail(const char *file, uint32_t line,
                                 const char *msg) {
  navtest_write_P(_NT_PSTR(_NT_RED "  FAIL: " _NT_RST));
  navtest_write_P(file);
  navtest_write_P(_NT_PSTR(":"));
  _navtest_print_uint32(line);
  navtest_write_P(_NT_PSTR(" -- "));
  navtest_write_P(msg);
  navtest_write_P(_NT_PSTR("\r\n"));
  _navtest.failures++;
}

static inline int _navtest_end_impl(void) {
  navtest_write_P(_NT_PSTR("  ["));
  _navtest_print_uint32(_navtest.tests);
  navtest_write_P(_NT_PSTR(" tests | "));
  _navtest_print_uint32(_navtest.failures);
  navtest_write_P(_NT_PSTR(" failed | "));
  _navtest_print_uint32(_navtest.passes);
  navtest_write_P(_NT_PSTR(" passed]\r\n"));
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
    uint32_t _prev_failures = _navtest.failures;                               \
    setUp();                                                                   \
    navtest_write_P(_NT_PSTR(_NT_CYAN "  >> " _NT_BOLD #fn _NT_RST "\r\n"));   \
    fn();                                                                      \
    if (_navtest.failures == _prev_failures) {                                 \
      _navtest.passes++;                                                       \
      navtest_write_P(_NT_PSTR(_NT_BOLD _NT_GREEN "  PASS" _NT_RST "\r\n"));   \
    }                                                                          \
    tearDown();                                                                \
  } while (0)

/* -------------------------------------------------------------------------
 * Suite registry — replaces per-driver RUN_TEST boilerplate in main.c.
 *
 * Each driver test file declares a `const navtest_suite_t test_<x>_suite`
 * pointing at its case table; `tests/main.c` walks an array of suite ptrs.
 * ---------------------------------------------------------------------- */

typedef void (*navtest_fn_t)(void);
typedef void (*navtest_hook_t)(void);

typedef struct {
  navtest_fn_t fn;
  const char *name;
} navtest_case_t;

typedef struct {
  const char *name;
  const navtest_case_t *cases;
  size_t count;
  navtest_hook_t between; /* optional: runs between cases; NULL for none */
} navtest_suite_t;

/* On AVR, case names live in PROGMEM: each test file declares one
 * static const char[] per case via NAVTEST_CASE_DECL(fn), and
 * NAVTEST_CASE(fn) points at it. Two-macro split is forced by the
 * language — _NT_PSTR expands to a statement-expression and can't
 * appear in a file-scope initializer, so the PROGMEM slot has to be
 * a named array declared at file scope first.
 *
 * On every other arch, NAVTEST_CASE_DECL collapses to an inert
 * declaration (so the trailing ';' is still valid) and NAVTEST_CASE
 * uses plain stringification.
 *
 * navtest_run_suite reads c->name via navtest_write_P, which is the
 * AVR PROGMEM reader on AVR and #defined to navtest_write everywhere
 * else — so the suite walker is portable as-is.
 *
 * Usage:
 *   NAVTEST_CASE_DECL(test_foo);
 *   NAVTEST_CASE_DECL(test_bar);
 *   static const navtest_case_t cases[] = {
 *     NAVTEST_CASE(test_foo),
 *     NAVTEST_CASE(test_bar),
 *   };
 */
#if defined(__AVR__)
#  define NAVTEST_CASE_DECL(fn)                                                \
     static const char _navtest_name_##fn[] __attribute__((__progmem__)) = #fn
#  define NAVTEST_CASE(fn) {(fn), _navtest_name_##fn}
#else
#  define NAVTEST_CASE_DECL(fn) extern const char _navtest_unused_##fn[]
#  define NAVTEST_CASE(fn) {(fn), #fn}
#endif

/** Run every case in @p suite, printing a banner + summary. Returns failures. */
int navtest_run_suite(const navtest_suite_t *suite);

/* -------------------------------------------------------------------------
 * Assertion macros
 * ---------------------------------------------------------------------- */

#define TEST_ASSERT_EQUAL_UINT32(expected, actual)                             \
  do {                                                                         \
    uint32_t _e = (uint32_t)(expected);                                        \
    uint32_t _a = (uint32_t)(actual);                                          \
    if (_e != _a) {                                                            \
      navtest_write_P(_NT_PSTR("  Expected: "));                               \
      _navtest_print_uint32(_e);                                               \
      navtest_write_P(_NT_PSTR("  Got: "));                                    \
      _navtest_print_uint32(_a);                                               \
      navtest_write_P(_NT_PSTR("\r\n"));                                       \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_EQUAL_UINT32"));                     \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                              \
  do {                                                                         \
    if ((void *)(ptr) == (void *)0)                                            \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_NOT_NULL: pointer is NULL"));        \
  } while (0)

#define TEST_ASSERT_TRUE(cond)                                                 \
  do {                                                                         \
    if (!(cond))                                                               \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_TRUE: condition is false"));         \
  } while (0)

#define TEST_ASSERT_FALSE(cond)                                                \
  do {                                                                         \
    if ((cond))                                                                \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_FALSE: condition is true"));         \
  } while (0)

/** Assert that all bits set in @p mask are also set in @p val */
#define TEST_ASSERT_BITS_HIGH(mask, val)                                       \
  do {                                                                         \
    uint32_t _m = (uint32_t)(mask);                                            \
    uint32_t _v = (uint32_t)(val);                                             \
    if ((_v & _m) != _m) {                                                     \
      navtest_write_P(_NT_PSTR("  Mask: "));                                   \
      _navtest_print_uint32(_m);                                               \
      navtest_write_P(_NT_PSTR("  Val:  "));                                   \
      _navtest_print_uint32(_v);                                               \
      navtest_write_P(_NT_PSTR("\r\n"));                                       \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_BITS_HIGH: bits not set"));          \
    }                                                                          \
  } while (0)

/** Assert that all bits set in @p mask are cleared in @p val */
#define TEST_ASSERT_BITS_LOW(mask, val)                                        \
  do {                                                                         \
    uint32_t _m = (uint32_t)(mask);                                            \
    uint32_t _v = (uint32_t)(val);                                             \
    if ((_v & _m) != 0) {                                                      \
      navtest_write_P(_NT_PSTR("  Mask: "));                                   \
      _navtest_print_uint32(_m);                                               \
      navtest_write_P(_NT_PSTR("  Val:  "));                                   \
      _navtest_print_uint32(_v);                                               \
      navtest_write_P(_NT_PSTR("\r\n"));                                       \
      _navtest_fail(_NT_PSTR(__FILE__), __LINE__,                              \
                    _NT_PSTR("TEST_ASSERT_BITS_LOW: bits not cleared"));       \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVTEST_H */
