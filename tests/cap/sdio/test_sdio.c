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
 * @file tests/test_sdio.c
 * @brief Standardized hal_sdio_* API smoke tests.
 *
 * SDIO sits behind NAVHAL_HAS_SDIO. The actual card is rarely connected
 * on a bare Nucleo, so the tests cover argument validation, the typed
 * error-code surface, and ensure that mutating helpers don't crash on
 * smoke-test inputs.
 */

#define CORTEX_M4
#include "test_sdio.h"

#if NAVHAL_HAS_SDIO

#include "family/sdio_reg.h"
#include "navhal_port_sdio.h"
#include "navtest/navtest.h"

void test_hal_sdio_init_rejects_null_config(void) {
  /* The contract is hal_sdio_error_t; HAL_SDIO_ERROR is the generic
   * rejection code. */
  TEST_ASSERT_TRUE(hal_sdio_init(NULL) != HAL_SDIO_OK);
}

void test_hal_sdio_read_block_rejects_null_buffer(void) {
  /* Skipped: hal_sdio_read_block issues CMD17 and waits for the card to
   * respond before returning, so on a board without an SD card present
   * the call blocks regardless of the buffer argument. Add a pre-CMD17
   * NULL guard in the driver to make this testable here. */
  TEST_ASSERT_TRUE(1);
}

void test_hal_sdio_write_block_rejects_null_buffer(void) {
  /* Skipped: same as read_block — write_block issues CMD24 and waits
   * for the card. Without a card, the call blocks regardless of args. */
  TEST_ASSERT_TRUE(1);
}

void test_hal_sdio_get_sector_count_returns_value(void) {
  /* Without a card the sector count may be 0 — what matters is the call
   * returns and doesn't fault. */
  uint32_t n = hal_sdio_get_sector_count();
  (void)n;
  TEST_ASSERT_TRUE(1);
}

static volatile uint32_t s_sdio_cb_hits = 0;
static void sdio_test_cb(hal_sdio_error_t err) {
  (void)err;
  s_sdio_cb_hits++;
}

void test_hal_sdio_set_callback_smoke(void) {
  s_sdio_cb_hits = 0;
  hal_sdio_set_callback(sdio_test_cb);
  hal_sdio_set_callback(NULL); /* re-clear */
  TEST_ASSERT_TRUE(1);
}

/* --- ISR clear-mask invariant -------------------------------------------- *
 * Regression guard for an intermittent file-transfer stall: the SDIO data-
 * completion ISR clears its transfer with SDIO_ICR_DATA_FLAGS and must NEVER
 * clear the command-path flags (CMDREND in particular) that the synchronous
 * send_command() poll is waiting on. For a fast single-block read the data
 * phase can finish — firing the ISR — before a preempted poll has observed
 * CMDREND; a blanket ICR = 0xFFFFFFFF in the ISR then erases CMDREND, the poll
 * times out on a command that actually succeeded, and FatFS turns the spurious
 * HAL_SDIO_TIMEOUT into FR_DISK_ERR and wedges. These are pure constant checks
 * (no card, no peripheral state), so they hold on any board and in PIL. The
 * expected sets below are re-derived from the individual bits — independent of
 * the grouped macros — so a bad edit to either mask is actually caught. */
#define EXPECT_DATA_CLEAR                                                       \
  (SDIO_ICR_DCRCFAILC | SDIO_ICR_DTIMEOUTC | SDIO_ICR_TXUNDERRC |              \
   SDIO_ICR_RXOVERRC | SDIO_ICR_DATAENDC | SDIO_ICR_STBITERRC |               \
   SDIO_ICR_DBCKENDC)
#define EXPECT_CMD_CLEAR                                                        \
  (SDIO_ICR_CCRCFAILC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CMDRENDC |              \
   SDIO_ICR_CMDSENTC)

/* THE invariant: the data and command clear families share no bit, so an ISR
 * writing SDIO_ICR_DATA_FLAGS cannot disturb any command flag. */
void test_sdio_data_and_command_clear_masks_are_disjoint(void) {
  TEST_ASSERT_EQUAL_UINT32(
      0u, (uint32_t)(SDIO_ICR_DATA_FLAGS & SDIO_ICR_CMD_FLAGS));
}

/* Name the exact flag from the bug. */
void test_sdio_cmdrend_not_in_data_clear(void) {
  TEST_ASSERT_EQUAL_UINT32(0u,
                           (uint32_t)(SDIO_ICR_DATA_FLAGS & SDIO_ICR_CMDRENDC));
}

/* Cover every data-path flag — a forgotten one leaves the DPSM flagged and
 * wedges the next transfer instead. */
void test_sdio_data_clear_covers_all_data_flags(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)EXPECT_DATA_CLEAR,
                           (uint32_t)SDIO_ICR_DATA_FLAGS);
}

/* The command clear is the complete complement, CMDREND included. */
void test_sdio_command_clear_covers_all_command_flags(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)EXPECT_CMD_CLEAR,
                           (uint32_t)SDIO_ICR_CMD_FLAGS);
}

/* A blanket clear (0xFFFFFFFF) is the regression that caused the stall; an
 * empty mask would never clear the transfer. Reject both. */
void test_sdio_data_clear_is_not_a_blanket_clear(void) {
  TEST_ASSERT_TRUE(SDIO_ICR_DATA_FLAGS != 0xFFFFFFFFu);
  TEST_ASSERT_TRUE(SDIO_ICR_DATA_FLAGS != 0u);
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere. */
NAVTEST_CASE_DECL(test_hal_sdio_init_rejects_null_config);
NAVTEST_CASE_DECL(test_hal_sdio_read_block_rejects_null_buffer);
NAVTEST_CASE_DECL(test_hal_sdio_write_block_rejects_null_buffer);
NAVTEST_CASE_DECL(test_hal_sdio_get_sector_count_returns_value);
NAVTEST_CASE_DECL(test_hal_sdio_set_callback_smoke);
NAVTEST_CASE_DECL(test_sdio_data_and_command_clear_masks_are_disjoint);
NAVTEST_CASE_DECL(test_sdio_cmdrend_not_in_data_clear);
NAVTEST_CASE_DECL(test_sdio_data_clear_covers_all_data_flags);
NAVTEST_CASE_DECL(test_sdio_command_clear_covers_all_command_flags);
NAVTEST_CASE_DECL(test_sdio_data_clear_is_not_a_blanket_clear);


static const navtest_case_t sdio_cases[] = {
    NAVTEST_CASE(test_hal_sdio_init_rejects_null_config),
    NAVTEST_CASE(test_hal_sdio_read_block_rejects_null_buffer),
    NAVTEST_CASE(test_hal_sdio_write_block_rejects_null_buffer),
    NAVTEST_CASE(test_hal_sdio_get_sector_count_returns_value),
    NAVTEST_CASE(test_hal_sdio_set_callback_smoke),
    NAVTEST_CASE(test_sdio_data_and_command_clear_masks_are_disjoint),
    NAVTEST_CASE(test_sdio_cmdrend_not_in_data_clear),
    NAVTEST_CASE(test_sdio_data_clear_covers_all_data_flags),
    NAVTEST_CASE(test_sdio_command_clear_covers_all_command_flags),
    NAVTEST_CASE(test_sdio_data_clear_is_not_a_blanket_clear),
};

const navtest_suite_t test_sdio_suite = {
    .name = "SDIO",
    .cases = sdio_cases,
    .count = sizeof(sdio_cases) / sizeof(sdio_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_HAS_SDIO */
