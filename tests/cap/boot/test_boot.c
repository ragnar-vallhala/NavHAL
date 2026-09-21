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
 * @file test_boot.c
 * @brief On-target tests for the boot block and console watcher.
 *
 * @details
 * **Nothing here calls hal_boot_request.** It resets the board, which would
 * end the run at whatever case reached it and look like a hang rather than a
 * result. Every case that needs a completed match therefore disables entry
 * first, which exercises the match and the refusal and leaves the board
 * running.
 *
 * What the host suite cannot check is here instead: the matcher's logic is
 * tested in tests/host/test_boot_sniffer.c against a stubbed reset, but
 * whether `_sboot` actually survives a reset is a property of the linker
 * script, and only a real image can answer it.
 */

#include "common/hal_boot.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_BOOT_SNIFFER

/* The bounds of the region startup zeroes. Each toolchain names them itself:
 * avr-libc's own linker script calls them __bss_start/__bss_end, while this
 * project's Cortex-M scripts emit _sbss/_ebss for the zero table boot.c walks.
 * The property is the same on both, only the spelling differs. */
#if defined(__AVR__)
extern char __bss_start;
extern char __bss_end;
#define BOOT_BSS_START ((uintptr_t)&__bss_start)
#define BOOT_BSS_END ((uintptr_t)&__bss_end)
#else
extern uint32_t _sbss;
extern uint32_t _ebss;
#define BOOT_BSS_START ((uintptr_t)&_sbss)
#define BOOT_BSS_END ((uintptr_t)&_ebss)
#endif

/**
 * The one thing that cannot be tested anywhere but on a real image: the boot
 * block has to sit outside the range Reset_Handler zeroes, or the request it
 * carries is wiped before the next boot can read it. That failure is silent
 * and looks like "the watcher does nothing", which is a bad afternoon.
 */
void test_boot_block_escapes_the_zeroed_region(void) {
  uintptr_t block = (uintptr_t)&_sboot;
  uintptr_t bss_start = BOOT_BSS_START;
  uintptr_t bss_end = BOOT_BSS_END;

  TEST_ASSERT_TRUE(block < bss_start || block >= bss_end);
  TEST_ASSERT_TRUE(block + sizeof(hal_boot_block_t) <= bss_start ||
                   block >= bss_end);
}

void test_boot_block_is_valid_after_init(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_block_init());
  TEST_ASSERT_TRUE(hal_boot_block_valid());
}

void test_boot_entry_gate_round_trips(void) {
  bool was = hal_boot_entry_is_disabled();

  hal_boot_entry_disable();
  TEST_ASSERT_TRUE(hal_boot_entry_is_disabled());
  hal_boot_entry_enable();
  TEST_ASSERT_FALSE(hal_boot_entry_is_disabled());

  if (was) {
    hal_boot_entry_disable();
  }
}

void test_boot_match_is_refused_while_entry_disabled(void) {
  /* The full sequence, on a board that must keep running. Entry is disabled
   * first, so the match completes and is refused instead of resetting. */
  uint32_t before = hal_boot_get_request();

  hal_boot_entry_disable();
  hal_boot_match_reset();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);

  /* Still here, and no request left behind for an unrelated reset to honour. */
  TEST_ASSERT_EQUAL_UINT32(before, hal_boot_get_request());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request());

  hal_boot_entry_enable();
  hal_boot_match_reset();
}

void test_boot_match_ignores_console_traffic(void) {
  static const uint8_t chatter[] = "NavHAL test run\r\n";

  hal_boot_entry_disable();
  hal_boot_match_reset();
  hal_boot_feed(chatter, (uint16_t)(sizeof chatter - 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
  hal_boot_entry_enable();
  hal_boot_match_reset();
}

NAVTEST_CASE_DECL(test_boot_block_escapes_the_zeroed_region);
NAVTEST_CASE_DECL(test_boot_block_is_valid_after_init);
NAVTEST_CASE_DECL(test_boot_entry_gate_round_trips);
NAVTEST_CASE_DECL(test_boot_match_is_refused_while_entry_disabled);
NAVTEST_CASE_DECL(test_boot_match_ignores_console_traffic);

static const navtest_case_t boot_cases[] = {
    NAVTEST_CASE(test_boot_block_escapes_the_zeroed_region),
    NAVTEST_CASE(test_boot_block_is_valid_after_init),
    NAVTEST_CASE(test_boot_entry_gate_round_trips),
    NAVTEST_CASE(test_boot_match_is_refused_while_entry_disabled),
    NAVTEST_CASE(test_boot_match_ignores_console_traffic),
};

const navtest_suite_t test_boot_suite = {
    .name = "BOOT",
    .cases = boot_cases,
    .count = sizeof(boot_cases) / sizeof(boot_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_BOOT_SNIFFER */
