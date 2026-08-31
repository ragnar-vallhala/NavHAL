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
 * @file test_usb_cdc.c
 * @brief On-target tests for the USB CDC-ACM device driver.
 *
 * @details
 * Device-free, and that is most of the point: with no host the driver has to
 * report a closed port and refuse every transfer rather than block, stall or
 * touch a peripheral it never brought up. A board whose USB pins go nowhere is
 * the same case as a cable that is not plugged in, so these run anywhere.
 *
 * The transfer paths themselves need a host to mean anything and are exercised
 * by samples/cortex-m/34_hal_usb_cdc.
 */

#include "test_usb_cdc.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_USB_CDC
#include "common/hal_usb_cdc.h"

void test_usb_cdc_not_connected_without_host(void) {
  TEST_ASSERT_FALSE(hal_usb_cdc_connected());
}

void test_usb_cdc_write_refused_without_host(void) {
  const uint8_t byte = 'x';
  /* Not INVALID_ARG: the arguments are fine, there is just no configured
   * host to send to. A caller retries on one and gives up on the other. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED,
                           hal_usb_cdc_write(&byte, 1));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED,
                           hal_usb_cdc_write_string("x"));
}

void test_usb_cdc_write_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_usb_cdc_write(NULL, 1));
}

void test_usb_cdc_write_string_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_usb_cdc_write_string(NULL));
}

void test_usb_cdc_zero_length_write_is_not_an_arg_error(void) {
  /* Nothing to dereference, so the NULL is not the complaint — the missing
   * host is. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED, hal_usb_cdc_write(NULL, 0));
}

void test_usb_cdc_notify_refused_without_host(void) {
  /* Must return, not spin on the notification endpoint's busy flag. */
  TEST_ASSERT_EQUAL_UINT32(
      HAL_ERR_NOT_INITIALIZED,
      hal_usb_cdc_notify_serial_state(HAL_USB_CDC_STATE_DCD));
}

void test_usb_cdc_read_empty_returns_zero(void) {
  uint8_t buf[8];
  TEST_ASSERT_EQUAL_UINT32(0u, hal_usb_cdc_available());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_usb_cdc_read(buf, sizeof(buf)));
}

void test_usb_cdc_read_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32(0u, hal_usb_cdc_read(NULL, 8));
}

void test_usb_cdc_line_state_clear_without_host(void) {
  /* DTR clear is what tells an application not to write yet. */
  TEST_ASSERT_EQUAL_UINT32(0u, hal_usb_cdc_get_line_state());
}

void test_usb_cdc_line_coding_defaults_to_115200_8n1(void) {
  /* The host overwrites this at SET_LINE_CODING; until then a caller reading
   * it back must get a usable setting rather than zeroes. */
  hal_usb_cdc_line_coding_t lc = {0};
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_usb_cdc_get_line_coding(&lc));
  TEST_ASSERT_EQUAL_UINT32(115200u, lc.baudrate);
  TEST_ASSERT_EQUAL_UINT32(8u, lc.data_bits);
  TEST_ASSERT_EQUAL_UINT32(0u, lc.stop_bits);
  TEST_ASSERT_EQUAL_UINT32(0u, lc.parity);
  TEST_ASSERT_EQUAL_UINT32(115200u, hal_usb_cdc_get_baudrate());
}

void test_usb_cdc_line_coding_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_usb_cdc_get_line_coding(NULL));
}

void test_usb_cdc_break_is_zero_without_host(void) {
  TEST_ASSERT_EQUAL_UINT32(0u, hal_usb_cdc_get_break_ms());
}

void test_usb_cdc_rx_callback_accepts_null(void) {
  /* NULL is how a caller detaches. */
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_usb_cdc_set_rx_callback(NULL));
}

void test_usb_cdc_init_requires_48mhz(void) {
  /* The test harness leaves the PLL alone, so the 48 MHz the OTG_FS core needs
   * is not there. Enumerating anyway would put the device on the bus at the
   * wrong bit rate, so init has to refuse — and refuse before it powers up the
   * transceiver or claims PA11/PA12. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED, hal_usb_cdc_init());
  TEST_ASSERT_FALSE(hal_usb_cdc_connected());
}

void test_usb_cdc_deinit_before_init_is_a_noop(void) {
  /* Reaching the assert is the test: every register deinit touches sits behind
   * the OTG_FS clock gate, and reading one with the gate shut is a bus fault. */
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_usb_cdc_deinit());
}

/* -------------------- Suite -------------------- */

NAVTEST_CASE_DECL(test_usb_cdc_not_connected_without_host);
NAVTEST_CASE_DECL(test_usb_cdc_write_refused_without_host);
NAVTEST_CASE_DECL(test_usb_cdc_write_rejects_null);
NAVTEST_CASE_DECL(test_usb_cdc_write_string_rejects_null);
NAVTEST_CASE_DECL(test_usb_cdc_zero_length_write_is_not_an_arg_error);
NAVTEST_CASE_DECL(test_usb_cdc_notify_refused_without_host);
NAVTEST_CASE_DECL(test_usb_cdc_read_empty_returns_zero);
NAVTEST_CASE_DECL(test_usb_cdc_read_rejects_null);
NAVTEST_CASE_DECL(test_usb_cdc_line_state_clear_without_host);
NAVTEST_CASE_DECL(test_usb_cdc_line_coding_defaults_to_115200_8n1);
NAVTEST_CASE_DECL(test_usb_cdc_line_coding_rejects_null);
NAVTEST_CASE_DECL(test_usb_cdc_break_is_zero_without_host);
NAVTEST_CASE_DECL(test_usb_cdc_rx_callback_accepts_null);
NAVTEST_CASE_DECL(test_usb_cdc_init_requires_48mhz);
NAVTEST_CASE_DECL(test_usb_cdc_deinit_before_init_is_a_noop);

static const navtest_case_t usb_cdc_cases[] = {
    NAVTEST_CASE(test_usb_cdc_not_connected_without_host),
    NAVTEST_CASE(test_usb_cdc_write_refused_without_host),
    NAVTEST_CASE(test_usb_cdc_write_rejects_null),
    NAVTEST_CASE(test_usb_cdc_write_string_rejects_null),
    NAVTEST_CASE(test_usb_cdc_zero_length_write_is_not_an_arg_error),
    NAVTEST_CASE(test_usb_cdc_notify_refused_without_host),
    NAVTEST_CASE(test_usb_cdc_read_empty_returns_zero),
    NAVTEST_CASE(test_usb_cdc_read_rejects_null),
    NAVTEST_CASE(test_usb_cdc_line_state_clear_without_host),
    NAVTEST_CASE(test_usb_cdc_line_coding_defaults_to_115200_8n1),
    NAVTEST_CASE(test_usb_cdc_line_coding_rejects_null),
    NAVTEST_CASE(test_usb_cdc_break_is_zero_without_host),
    NAVTEST_CASE(test_usb_cdc_rx_callback_accepts_null),
    NAVTEST_CASE(test_usb_cdc_init_requires_48mhz),
    NAVTEST_CASE(test_usb_cdc_deinit_before_init_is_a_noop),
};

const navtest_suite_t test_usb_cdc_suite = {
    .name = "USB_CDC",
    .cases = usb_cdc_cases,
    .count = sizeof(usb_cdc_cases) / sizeof(usb_cdc_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_USB_CDC */
