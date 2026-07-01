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
 * @file tests/cap/tcm/test_tcm.c
 * @brief On-target tests for the Cortex-M7 TCM placement attributes.
 *
 * @details
 * Places a function in ITCM and data in DTCM via the hal_tcm.h attributes, then
 * checks (a) the symbols land in the TCM address ranges, (b) the ITCM function
 * actually executes (proving the startup flash->ITCM copy ran), and (c) the
 * initialized DTCM value was copied and the NOINIT buffer was zeroed. Runs only
 * where NAVHAL_CONFIG_USE_TCM is set (Cortex-M7).
 */

#include "test_tcm.h"
#include "common/hal_features.h"
#include "common/hal_tcm.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_USE_TCM

#define ITCM_BASE 0x00000000UL
#define ITCM_END  0x00004000UL /* 16 KB */
#define DTCM_BASE 0x20000000UL
#define DTCM_END  0x20020000UL /* 128 KB */

/* Explicit placement — the point of the whole feature. */
NAVHAL_ITCM static int tcm_add(int a, int b) { return a + b; }
NAVHAL_DTCM static volatile uint32_t dtcm_init = 0xABCD1234u;
NAVHAL_DTCM_NOINIT static volatile uint32_t dtcm_buf[4];

void test_itcm_func_placed_and_runs(void) {
  uintptr_t a = (uintptr_t)&tcm_add;
  TEST_ASSERT_TRUE(a >= 1u && a < ITCM_END); /* linked into ITCM */
  /* Executing it proves the reset-time flash->ITCM copy populated the code. */
  TEST_ASSERT_EQUAL_UINT32(7u, (uint32_t)tcm_add(3, 4));
}

void test_dtcm_init_copied(void) {
  uintptr_t a = (uintptr_t)&dtcm_init;
  TEST_ASSERT_TRUE(a >= DTCM_BASE && a < DTCM_END); /* linked into DTCM */
  TEST_ASSERT_EQUAL_UINT32(0xABCD1234u, dtcm_init); /* startup copy ran */
  dtcm_init = 0x55AA55AAu;
  TEST_ASSERT_EQUAL_UINT32(0x55AA55AAu, dtcm_init); /* writable */
}

void test_dtcm_noinit_zeroed(void) {
  uintptr_t a = (uintptr_t)&dtcm_buf[0];
  TEST_ASSERT_TRUE(a >= DTCM_BASE && a < DTCM_END); /* linked into DTCM */
  TEST_ASSERT_EQUAL_UINT32(0u, dtcm_buf[0]);        /* startup zero ran */
  TEST_ASSERT_EQUAL_UINT32(0u, dtcm_buf[3]);
  dtcm_buf[0] = 0xDEADBEEFu;
  TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, dtcm_buf[0]); /* writable */
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (never links on AVR —
 * TCM is Cortex-M7 only — but keep the portable idiom). */
NAVTEST_CASE_DECL(test_itcm_func_placed_and_runs);
NAVTEST_CASE_DECL(test_dtcm_init_copied);
NAVTEST_CASE_DECL(test_dtcm_noinit_zeroed);

static const navtest_case_t tcm_cases[] = {
    NAVTEST_CASE(test_itcm_func_placed_and_runs),
    NAVTEST_CASE(test_dtcm_init_copied),
    NAVTEST_CASE(test_dtcm_noinit_zeroed),
};

const navtest_suite_t test_tcm_suite = {
    .name = "TCM (cap)",
    .cases = tcm_cases,
    .count = sizeof(tcm_cases) / sizeof(tcm_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_USE_TCM */
