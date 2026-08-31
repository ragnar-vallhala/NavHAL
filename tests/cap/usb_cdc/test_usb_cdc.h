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

#ifndef TEST_USB_CDC_H
#define TEST_USB_CDC_H

#include "common/hal_features.h"
#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC

void test_usb_cdc_not_connected_without_host(void);
void test_usb_cdc_write_refused_without_host(void);
void test_usb_cdc_write_rejects_null(void);
void test_usb_cdc_write_string_rejects_null(void);
void test_usb_cdc_zero_length_write_is_not_an_arg_error(void);
void test_usb_cdc_notify_refused_without_host(void);
void test_usb_cdc_read_empty_returns_zero(void);
void test_usb_cdc_read_rejects_null(void);
void test_usb_cdc_line_state_clear_without_host(void);
void test_usb_cdc_line_coding_defaults_to_115200_8n1(void);
void test_usb_cdc_line_coding_rejects_null(void);
void test_usb_cdc_break_is_zero_without_host(void);
void test_usb_cdc_rx_callback_accepts_null(void);
void test_usb_cdc_init_requires_48mhz(void);
void test_usb_cdc_deinit_before_init_is_a_noop(void);

extern const navtest_suite_t test_usb_cdc_suite;

#endif /* NAVHAL_CONFIG_DRV_USB_CDC */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_USB_CDC_H
