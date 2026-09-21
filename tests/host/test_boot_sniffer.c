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
 * @file test_boot_sniffer.c
 * @brief Host (SIL) tests for the boot block and the console watcher.
 *
 * @details
 * The matcher is pure logic, so this is the tier that can actually exercise
 * it: feeding a real board the sequence proves one arrangement of bytes
 * works, while here every split of every stream can be tried in a second.
 *
 * hal_system_reset is stubbed to count calls instead of resetting, which is
 * what makes the "did it act" assertions possible at all.
 */

#include "common/hal_boot.h"
#include "common/hal_reset.h"
#include "navtest/navtest.h"
#include <string.h>

/* ---------------------------------------------------------------------------
 * Stand-in for the reset. The real one does not return; this records the call
 * so a test can assert the watcher acted, and returns so the test can carry
 * on. hal_boot_request() treats a returning reset as failure, which is why it
 * reports HAL_ERR below -- that is the stub's doing, not a defect.
 * ------------------------------------------------------------------------- */
static uint32_t s_resets;
static uint32_t s_prepares;

hal_status_t hal_system_reset(void) {
  s_resets++;
  return HAL_OK;
}

/* The other hal_reset_* entry points are not linked in; declare nothing. */

static void prepare_hook(void) { s_prepares++; }

static void fresh(void) {
  s_resets = 0u;
  s_prepares = 0u;
  hal_boot_match_reset();
  hal_boot_entry_enable();
  hal_boot_set_prepare(NULL);
  memset((void *)&_sboot, 0, sizeof _sboot);
  hal_boot_block_init();
}

/* ---------------------------------------------------------------------------
 * The sequence's own property
 * ------------------------------------------------------------------------- */

/* The matcher resyncs by retrying one byte against seq[0]. That is correct
 * only when no proper prefix of the sequence is also a suffix of it. Asserting
 * it here means a future edit to the constant cannot quietly break matching
 * for every sequence that arrives mid-stream -- a failure that would never
 * show up in a test that only feeds the sequence from a clean start. */
void test_boot_seq_has_no_prefix_suffix_overlap(void) {
  for (uint8_t k = 1u; k < HAL_BOOT_SEQ_LEN; k++) {
    bool overlap = true;
    for (uint8_t i = 0u; i < k; i++) {
      if (hal_boot_seq[i] != hal_boot_seq[HAL_BOOT_SEQ_LEN - k + i]) {
        overlap = false;
        break;
      }
    }
    TEST_ASSERT_FALSE(overlap);
  }
}

void test_boot_seq_bytes_are_distinct(void) {
  /* Distinctness is how the property above is guaranteed by construction. */
  for (uint8_t i = 0u; i < HAL_BOOT_SEQ_LEN; i++) {
    for (uint8_t j = (uint8_t)(i + 1u); j < HAL_BOOT_SEQ_LEN; j++) {
      TEST_ASSERT_TRUE(hal_boot_seq[i] != hal_boot_seq[j]);
    }
  }
}

/* ---------------------------------------------------------------------------
 * Matching
 * ------------------------------------------------------------------------- */

void test_boot_match_fires_once_on_the_sequence(void) {
  fresh();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_LOADER,
                           hal_boot_get_request());
}

void test_boot_match_survives_every_split(void) {
  /* The whole point of a byte-at-a-time matcher: a DMA chunk boundary or a
   * CDC packet boundary can fall anywhere, and must change nothing. */
  for (uint8_t cut = 0u; cut <= HAL_BOOT_SEQ_LEN; cut++) {
    fresh();
    hal_boot_feed(hal_boot_seq, cut);
    hal_boot_feed(hal_boot_seq + cut, (uint16_t)(HAL_BOOT_SEQ_LEN - cut));
    TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
  }
}

void test_boot_match_one_byte_at_a_time(void) {
  fresh();
  for (uint8_t i = 0u; i < HAL_BOOT_SEQ_LEN; i++) {
    hal_boot_match_byte(hal_boot_seq[i]);
  }
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
}

void test_boot_match_ignores_traffic(void) {
  static const uint8_t chatter[] = "GPS fix 3D sats=11\r\nbattery 11.7V\r\n";
  fresh();
  hal_boot_feed(chatter, (uint16_t)(sizeof chatter - 1u));
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
}

void test_boot_match_finds_sequence_after_a_partial(void) {
  /* A truncated attempt then the real thing, with no gap. This is the case
   * the resync rule exists for, and the one a from-clean-start test misses. */
  uint8_t stream[HAL_BOOT_SEQ_LEN * 2u];
  fresh();
  memcpy(stream, hal_boot_seq, HAL_BOOT_SEQ_LEN - 1u); /* partial */
  memcpy(stream + HAL_BOOT_SEQ_LEN - 1u, hal_boot_seq, HAL_BOOT_SEQ_LEN);
  hal_boot_feed(stream, (uint16_t)(HAL_BOOT_SEQ_LEN * 2u - 1u));
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
}

void test_boot_match_recovers_from_a_wrong_byte_mid_sequence(void) {
  uint8_t stream[HAL_BOOT_SEQ_LEN + 3u];
  fresh();
  stream[0] = hal_boot_seq[0];
  stream[1] = hal_boot_seq[1];
  stream[2] = 0x11u; /* not in the sequence */
  memcpy(stream + 3u, hal_boot_seq, HAL_BOOT_SEQ_LEN);
  hal_boot_feed(stream, (uint16_t)(HAL_BOOT_SEQ_LEN + 3u));
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
}

void test_boot_match_reset_drops_a_partial(void) {
  fresh();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN - 1u);
  hal_boot_match_reset(); /* as after a link drop */
  hal_boot_feed(hal_boot_seq + HAL_BOOT_SEQ_LEN - 1u, 1u);
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);
}

void test_boot_feed_tolerates_null(void) {
  fresh();
  hal_boot_feed(NULL, 4u);
  hal_boot_feed(hal_boot_seq, 0u);
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);
}

/* ---------------------------------------------------------------------------
 * Entry policy
 * ------------------------------------------------------------------------- */

void test_boot_entry_disabled_refuses_to_act(void) {
  fresh();
  hal_boot_entry_disable();
  TEST_ASSERT_TRUE(hal_boot_entry_is_disabled());
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);

  /* Not rebooted, and -- just as important -- no request left behind to be
   * honoured by an unrelated reset later. */
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
}

void test_boot_entry_reenable_acts_on_the_next_sequence(void) {
  fresh();
  hal_boot_entry_disable();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);

  /* Matching continued while disabled, so the refused match must not have
   * left the matcher mid-sequence. */
  hal_boot_entry_enable();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
}

void test_boot_prepare_runs_before_the_reset(void) {
  fresh();
  hal_boot_set_prepare(prepare_hook);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR, (uint32_t)hal_boot_request());
  TEST_ASSERT_EQUAL_UINT32(1u, s_prepares);
  TEST_ASSERT_EQUAL_UINT32(1u, s_resets);
}

void test_boot_request_refused_when_disabled(void) {
  fresh();
  hal_boot_set_prepare(prepare_hook);
  hal_boot_entry_disable();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request());
  TEST_ASSERT_EQUAL_UINT32(0u, s_prepares); /* nothing ran, nothing reset */
  TEST_ASSERT_EQUAL_UINT32(0u, s_resets);
}

/* ---------------------------------------------------------------------------
 * Boot block
 * ------------------------------------------------------------------------- */

void test_boot_block_seeds_a_cold_boot(void) {
  memset((void *)&_sboot, 0, sizeof _sboot);
  TEST_ASSERT_FALSE(hal_boot_block_valid());
  hal_boot_block_init();
  TEST_ASSERT_TRUE(hal_boot_block_valid());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_boot_get_attempts());
}

void test_boot_block_init_preserves_a_request(void) {
  /* The request is written, then the reset happens, then init runs. If init
   * re-seeded a valid block the request would be lost on the way in, and the
   * board would boot straight back into the application it was rescued from. */
  fresh();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN);
  hal_boot_block_init(); /* stands in for the next boot */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_LOADER,
                           hal_boot_get_request());
}

void test_boot_block_rejects_garbage_that_looks_right(void) {
  /* Uninitialised SRAM can plausibly hold the magic; it will not hold the
   * magic together with a matching check. */
  memset((void *)&_sboot, 0, sizeof _sboot);
  _sboot.magic = HAL_BOOT_MAGIC;
  _sboot.request = HAL_BOOT_REQ_LOADER;
  _sboot.check = 0xFFFFFFFFu;
  TEST_ASSERT_FALSE(hal_boot_block_valid());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
}

void test_boot_block_rejects_all_ones(void) {
  memset((void *)&_sboot, 0xFF, sizeof _sboot);
  TEST_ASSERT_FALSE(hal_boot_block_valid());
}

void test_boot_clear_request_keeps_attempts(void) {
  fresh();
  hal_boot_feed(hal_boot_seq, HAL_BOOT_SEQ_LEN); /* writes a request */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_clear_request());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
  TEST_ASSERT_TRUE(hal_boot_block_valid());
}

void test_boot_mark_healthy_clears_attempts(void) {
  fresh();
  _sboot.attempts = 2u;
  _sboot.check = HAL_BOOT_MAGIC ^ _sboot.request ^ 2u ^ 0xA5A5A5A5u;
  TEST_ASSERT_EQUAL_UINT32(2u, hal_boot_get_attempts());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_mark_healthy());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_boot_get_attempts());
  TEST_ASSERT_TRUE(hal_boot_block_valid());
}

void test_boot_block_ops_refuse_an_invalid_block(void) {
  memset((void *)&_sboot, 0, sizeof _sboot);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_INITIALIZED,
                           (uint32_t)hal_boot_clear_request());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_INITIALIZED,
                           (uint32_t)hal_boot_mark_healthy());
}

NAVTEST_CASE_DECL(test_boot_seq_has_no_prefix_suffix_overlap);
NAVTEST_CASE_DECL(test_boot_seq_bytes_are_distinct);
NAVTEST_CASE_DECL(test_boot_match_fires_once_on_the_sequence);
NAVTEST_CASE_DECL(test_boot_match_survives_every_split);
NAVTEST_CASE_DECL(test_boot_match_one_byte_at_a_time);
NAVTEST_CASE_DECL(test_boot_match_ignores_traffic);
NAVTEST_CASE_DECL(test_boot_match_finds_sequence_after_a_partial);
NAVTEST_CASE_DECL(test_boot_match_recovers_from_a_wrong_byte_mid_sequence);
NAVTEST_CASE_DECL(test_boot_match_reset_drops_a_partial);
NAVTEST_CASE_DECL(test_boot_feed_tolerates_null);
NAVTEST_CASE_DECL(test_boot_entry_disabled_refuses_to_act);
NAVTEST_CASE_DECL(test_boot_entry_reenable_acts_on_the_next_sequence);
NAVTEST_CASE_DECL(test_boot_prepare_runs_before_the_reset);
NAVTEST_CASE_DECL(test_boot_request_refused_when_disabled);
NAVTEST_CASE_DECL(test_boot_block_seeds_a_cold_boot);
NAVTEST_CASE_DECL(test_boot_block_init_preserves_a_request);
NAVTEST_CASE_DECL(test_boot_block_rejects_garbage_that_looks_right);
NAVTEST_CASE_DECL(test_boot_block_rejects_all_ones);
NAVTEST_CASE_DECL(test_boot_clear_request_keeps_attempts);
NAVTEST_CASE_DECL(test_boot_mark_healthy_clears_attempts);
NAVTEST_CASE_DECL(test_boot_block_ops_refuse_an_invalid_block);

static const navtest_case_t boot_sniffer_cases[] = {
    NAVTEST_CASE(test_boot_seq_has_no_prefix_suffix_overlap),
    NAVTEST_CASE(test_boot_seq_bytes_are_distinct),
    NAVTEST_CASE(test_boot_match_fires_once_on_the_sequence),
    NAVTEST_CASE(test_boot_match_survives_every_split),
    NAVTEST_CASE(test_boot_match_one_byte_at_a_time),
    NAVTEST_CASE(test_boot_match_ignores_traffic),
    NAVTEST_CASE(test_boot_match_finds_sequence_after_a_partial),
    NAVTEST_CASE(test_boot_match_recovers_from_a_wrong_byte_mid_sequence),
    NAVTEST_CASE(test_boot_match_reset_drops_a_partial),
    NAVTEST_CASE(test_boot_feed_tolerates_null),
    NAVTEST_CASE(test_boot_entry_disabled_refuses_to_act),
    NAVTEST_CASE(test_boot_entry_reenable_acts_on_the_next_sequence),
    NAVTEST_CASE(test_boot_prepare_runs_before_the_reset),
    NAVTEST_CASE(test_boot_request_refused_when_disabled),
    NAVTEST_CASE(test_boot_block_seeds_a_cold_boot),
    NAVTEST_CASE(test_boot_block_init_preserves_a_request),
    NAVTEST_CASE(test_boot_block_rejects_garbage_that_looks_right),
    NAVTEST_CASE(test_boot_block_rejects_all_ones),
    NAVTEST_CASE(test_boot_clear_request_keeps_attempts),
    NAVTEST_CASE(test_boot_mark_healthy_clears_attempts),
    NAVTEST_CASE(test_boot_block_ops_refuse_an_invalid_block),
};

const navtest_suite_t test_boot_sniffer_suite = {
    .name = "BOOT SNIFFER (host)",
    .cases = boot_sniffer_cases,
    .count = sizeof(boot_sniffer_cases) / sizeof(boot_sniffer_cases[0]),
    .between = NULL,
};
