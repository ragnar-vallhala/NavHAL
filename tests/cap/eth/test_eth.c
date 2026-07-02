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
 * @file tests/cap/eth/test_eth.c
 * @brief On-target tests for the Ethernet MAC (STM32F7).
 *
 * @details
 * Device-free: the tests need nothing plugged into the RJ45. The argument
 * contract runs everywhere. The MDIO check reads the on-board PHY's identity
 * register over the management bus — a valid, non-floating ID (not 0x0000 or
 * 0xFFFF) proves the MAC clocks, the RMII/MDIO GPIOs, and the management
 * interface are alive on silicon, without asserting a specific PHY part. A
 * frame round-trip needs a link partner and lives in the sample, not here.
 * Runs where NAVHAL_CONFIG_DRV_ETH is set.
 */

#include "test_eth.h"

#if NAVHAL_CONFIG_DRV_ETH

#include "navhal.h"
#include "navhal_port_eth.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdint.h>

/* PHY address of the on-board LAN8742 on the Nucleo-F767ZI (strap default). */
#define ETH_TEST_PHY_ADDR 0

static void print_hex16(uint16_t v) {
  static const char digits[] = "0123456789ABCDEF";
  char s[5] = {digits[(v >> 12) & 0xF], digits[(v >> 8) & 0xF],
              digits[(v >> 4) & 0xF], digits[v & 0xF], '\0'};
  navtest_write(s);
}

/* Argument-contract checks — no bus traffic, run everywhere. */
void test_eth_rejects_null_args(void) {
  uint16_t v = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_init(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_phy_read(ETH_PHY_BSR, NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_get_link(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_set_mac_address(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_get_mac_address(NULL));
  (void)v;
}

void test_eth_phy_id_readable(void) {
  NAVTEST_SKIP_ON_PIL(); /* Renode does not model the ETH MAC / MDIO. */

  hal_eth_config_t cfg = {
      .mac_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = ETH_TEST_PHY_ADDR,
      .auto_negotiation = true,
  };
  hal_status_t s = hal_eth_init(&cfg);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_INITIALIZED);

  uint16_t id1 = 0, id2 = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_eth_phy_read(ETH_PHY_ID1, &id1));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_eth_phy_read(ETH_PHY_ID2, &id2));

  navtest_write("[eth phy id=0x");
  print_hex16(id1);
  print_hex16(id2);
  navtest_write("]\r\n");

  /* A live MDIO bus reads the PHY's OUI-based identity; a dead one floats to
   * all-ones or reads back all-zeros. */
  TEST_ASSERT_TRUE(id1 != 0x0000 && id1 != 0xFFFF);
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (ETH is
 * Cortex-M7 only). */
NAVTEST_CASE_DECL(test_eth_rejects_null_args);
NAVTEST_CASE_DECL(test_eth_phy_id_readable);

static const navtest_case_t eth_cases[] = {
    NAVTEST_CASE(test_eth_rejects_null_args),
    NAVTEST_CASE(test_eth_phy_id_readable),
};

const navtest_suite_t test_eth_suite = {
    .name = "ETH (cap)",
    .cases = eth_cases,
    .count = sizeof(eth_cases) / sizeof(eth_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_ETH */
