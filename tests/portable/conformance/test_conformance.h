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
 * @file test_conformance.h
 * @brief HAL contract conformance suite — runs on every port (almost).
 *
 * Mechanical assertions of properties the HAL contract documents
 * in `include/common/` docstrings and `docs/api_standardization.md`.
 * A port that compiles + links + passes this suite implements the
 * v1 contract correctly; one that fails is provably non-conformant.
 *
 * Each test is **black-box** — it calls only the public hal_* API
 * and asserts behaviour from outside. No register access, no
 * vendor-specific includes. The whole file lives under
 * tests/portable/ for exactly that reason.
 *
 * Runs on AVR too: TEST_ASSERT_* macros and case names both land in
 * PROGMEM on AVR (assertion strings via `_NT_PSTR()`, case names via
 * the `NAVTEST_CASE_DECL` predeclaration trick), keeping the suite
 * well under the ATmega328P's 2 KB SRAM ceiling.
 */
#ifndef TEST_CONFORMANCE_H
#define TEST_CONFORMANCE_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status type contract — error codes are non-zero, HAL_OK is zero,
 * codes are pairwise distinct, and each fits in 8 bits (RPC ABI). */
void test_conformance_status_ok_is_zero(void);
void test_conformance_status_errors_distinct(void);
void test_conformance_status_fits_uint8(void);

/* HAL_OK_OR_RETURN macro contract — passes the status through on OK,
 * short-circuits on non-OK, and evaluates its argument exactly once. */
void test_conformance_hal_ok_or_return_passes_through(void);
void test_conformance_hal_ok_or_return_short_circuits(void);

/* Null-pointer contract — every public init function that takes a
 * pointer must reject NULL with HAL_ERR_INVALID_ARG, not crash; and
 * calling the NULL path twice in a row must return the same status
 * both times (no state corruption on the error branch). */
void test_conformance_uart_init_rejects_null(void);
void test_conformance_clock_init_rejects_null(void);
void test_conformance_dma_init_rejects_null(void);
void test_conformance_i2c_init_rejects_null(void);
void test_conformance_spi_init_rejects_null(void);
void test_conformance_pwm_init_rejects_null(void);
void test_conformance_sdio_init_rejects_null(void);
void test_conformance_gpio_init_rejects_null(void);
void test_conformance_null_init_is_idempotent(void);

/* Capability-flag contract — NAVHAL_CONFIG_DRV_X is always 0 or 1, never
 * unset (#ifdef NAVHAL_CONFIG_DRV_X yields a value; the macro is a contract,
 * not a feature switch). */
void test_conformance_cap_macros_are_defined(void);

/* Console contract — the getters agree with themselves, the route is one of
 * the two the contract defines, and a NULL string is a no-op. */
void test_conformance_console_getters_are_stable(void);
void test_conformance_console_write_ignores_null(void);

/* UART convenience writers — an instance that does not exist is rejected, and
 * read_char answers rather than waiting on a peripheral that is not there. */
void test_conformance_uart_writers_reject_bad_instance(void);
void test_conformance_uart_read_char_answers_on_bad_instance(void);

void test_conformance_crc_init_rejects_null(void);
void test_conformance_timebase_callback_accepts_null(void);
void test_conformance_wwdg_kick_needs_a_running_watchdog(void);

/* USB CDC — the getters answer with no host attached, a notification needs one,
 * and asking whether the device can come up answers the same twice. */
void test_conformance_usb_cdc_getters_are_stable(void);
void test_conformance_usb_cdc_set_rx_callback_accepts_null(void);
void test_conformance_usb_cdc_notify_needs_a_host(void);
void test_conformance_usb_cdc_init_answers_the_same_twice(void);

/* SDIO, and the diskio boundary FatFs is layered on — the slot is answerable
 * before init, an empty one has its own answer, and every disk entry point
 * rejects a drive that is not there before it touches a card. */
void test_conformance_sdio_card_present_is_stable(void);
void test_conformance_sdio_card_init_answers_for_an_empty_slot(void);
void test_conformance_sdio_get_response_rejects_bad_register(void);
void test_conformance_sdio_set_callback_accepts_null(void);
void test_conformance_disk_rejects_a_drive_that_does_not_exist(void);
void test_conformance_disk_rejects_a_zero_length_transfer(void);

/* RTC contract — the backup file is finite, the wakeup period has a documented
 * range, cancelling twice answers the same, and the clock source is one of the
 * four the enum defines. */
void test_conformance_rtc_backup_write_rejects_bad_index(void);
void test_conformance_rtc_set_wakeup_rejects_bad_period(void);
void test_conformance_rtc_cancel_is_idempotent(void);
void test_conformance_rtc_get_clock_is_a_documented_source(void);

void test_conformance_mpu_disable_region_rejects_bad_index(void);

/* Boot sniffer contract — the entry interlock holds (a request while entry is
 * disabled is refused, not obeyed), the block seals and validates, and console
 * traffic that is not the sequence never advances the match. */
void test_conformance_boot_block_init_validates(void);
void test_conformance_boot_getters_are_stable(void);
void test_conformance_boot_entry_gate_round_trips(void);
void test_conformance_boot_request_is_refused_while_disabled(void);
void test_conformance_boot_match_ignores_other_traffic(void);
void test_conformance_boot_clear_and_heal_need_a_valid_block(void);
void test_conformance_boot_set_prepare_accepts_null(void);

extern const navtest_suite_t test_conformance_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TEST_CONFORMANCE_H */
