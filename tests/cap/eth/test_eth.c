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
/* The data path, which needs a cable and so reports rather than fails without
 * one. Everything above this point answers on a bare board; this is the one
 * case whose result depends on what is plugged in, and it says which.
 *
 * It puts exactly one frame on the wire: 60 bytes to the broadcast address,
 * from the locally-administered MAC this suite already uses, with ethertype
 * 0x88B5 -- the ethertype IEEE reserves for local experimental use, so nothing
 * on the network is expected to parse or answer it.
 *
 * Receiving is reported, never asserted: a quiet segment owes this board no
 * traffic, and failing because the network was idle would make the suite a
 * test of the network. */
void test_eth_link_and_send(void) {
  NAVTEST_SKIP_ON_PIL(); /* Renode does not model the ETH MAC. */

  hal_eth_config_t cfg = {
      .mac_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = ETH_TEST_PHY_ADDR,
      .auto_negotiation = true,
  };
  hal_status_t s = hal_eth_init(&cfg);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_INITIALIZED);

  /* Auto-negotiation takes a second or two, and it restarts on the reset that
   * flashing causes -- init deliberately does not wait for it, so reading the
   * link once reports "down" on a board whose cable is fine. Poll instead.
   *
   * A spin rather than hal_delay_ms: nothing here guarantees a timebase has
   * been started, and hal_delay_ms never returns without one, which would take
   * the whole run down with no summary rather than one failed case. */
  hal_eth_link_t link = {0};
  for (uint32_t tries = 0u; tries < 500u; tries++) {
    if (hal_eth_get_link(&link) == HAL_OK && link.up) {
      break;
    }
    for (volatile uint32_t i = 0u; i < 200000u; i++) {
    }
  }

  if (!link.up) {
    navtest_write("[eth link=down -- data path not exercised]\r\n");
    TEST_ASSERT_TRUE(1);
    return;
  }

  navtest_write("[eth link=up ");
  navtest_write(link.speed == HAL_ETH_SPEED_100M ? "100M" : "10M");
  navtest_write(link.duplex == HAL_ETH_FULL_DUPLEX ? " full" : " half");
  navtest_write("]\r\n");

  /* A link means the MAC finished coming up, so start must now succeed --
   * the branch a cableless bench can only ever see refuse. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_eth_start());

  static uint8_t frame[HAL_ETH_MIN_FRAME_LEN];
  for (uint16_t i = 0; i < HAL_ETH_MIN_FRAME_LEN; i++) {
    frame[i] = 0x00;
  }
  for (uint8_t i = 0; i < 6; i++) {
    frame[i] = 0xFF; /* broadcast */
  }
  for (uint8_t i = 0; i < 6; i++) {
    frame[6 + i] = cfg.mac_addr[i];
  }
  frame[12] = 0x88; /* local experimental ethertype 1 */
  frame[13] = 0xB5;

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_eth_send(frame, sizeof frame));

  /* The RX half, polled rather than read once: hal_eth_receive takes the next
   * completed descriptor and does not block, so a single call right after the
   * send only ever reports what happened to be in the ring already. Any live
   * segment carries broadcast traffic within a second or so.
   *
   * Reported, never asserted -- a quiet or point-to-point segment owes this
   * board nothing, and failing then would make this a test of the network. */
  static uint8_t rx[HAL_ETH_MAX_FRAME_LEN];
  uint16_t rx_len = 0;
  for (uint32_t tries = 0u; tries < 400u; tries++) {
    if (hal_eth_receive(rx, sizeof rx, &rx_len) == HAL_OK && rx_len > 0) {
      break;
    }
    for (volatile uint32_t i = 0u; i < 200000u; i++) {
    }
  }
  if (rx_len > 0) {
    navtest_write("[eth rx=yes ");
    print_hex16(rx_len);
    navtest_write(" bytes]\r\n");
  } else {
    navtest_write("[eth rx=none -- quiet segment]\r\n");
  }
}

/* TX and RX proven without a peer, by looping the PHY back on itself: a frame
 * handed to the MAC comes out of the PHY, turns round inside it and arrives back
 * through the RX descriptors. That covers the descriptor rings, the ETHRAM
 * placement and the DPSM in both directions -- and it does so with nothing
 * plugged in, where the cable path above can only ever report what the network
 * happened to be doing.
 *
 * Auto-negotiation is off while looped back: there is no partner to negotiate
 * with, so the speed and duplex are stated rather than discovered. The PHY is
 * reset back to negotiating on the way out, or the next run would find a link
 * that never comes up. */
void test_eth_phy_loopback_round_trips(void) {
  NAVTEST_SKIP_ON_PIL(); /* Renode models no ETH MAC. */

  hal_eth_config_t cfg = {
      .mac_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = ETH_TEST_PHY_ADDR,
      .auto_negotiation = true,
  };
  hal_status_t s = hal_eth_init(&cfg);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_INITIALIZED);

  if (hal_eth_phy_write(ETH_PHY_BCR, ETH_PHY_BCR_LOOPBACK |
                                         ETH_PHY_BCR_SPEED_100 |
                                         ETH_PHY_BCR_FULLDUPLEX) != HAL_OK) {
    navtest_write("[eth phy loopback unavailable]\r\n");
    TEST_ASSERT_TRUE(1);
    return;
  }
  for (volatile uint32_t i = 0u; i < 400000u; i++) { /* let the PHY settle */
  }

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_eth_start());

  /* A marker in the payload, so what comes back is provably the frame that
   * went out rather than whatever was already sitting in the ring. */
  static const uint8_t marker[4] = {0x4E, 0x41, 0x56, 0x4C}; /* "NAVL" */
  static uint8_t tx[HAL_ETH_MIN_FRAME_LEN];
  for (uint16_t i = 0; i < HAL_ETH_MIN_FRAME_LEN; i++) {
    tx[i] = 0x00;
  }
  for (uint8_t i = 0; i < 6; i++) {
    tx[i] = 0xFF;
    tx[6 + i] = cfg.mac_addr[i];
  }
  tx[12] = 0x88;
  tx[13] = 0xB5;
  for (uint8_t i = 0; i < 4; i++) {
    tx[14 + i] = marker[i];
  }

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_eth_send(tx, sizeof tx));

  static uint8_t rx[HAL_ETH_MAX_FRAME_LEN];
  uint16_t rx_len = 0;
  bool got = false;
  for (uint32_t tries = 0u; tries < 200u && !got; tries++) {
    if (hal_eth_receive(rx, sizeof rx, &rx_len) == HAL_OK && rx_len >= 18u) {
      got = rx[12] == 0x88 && rx[13] == 0xB5 && rx[14] == marker[0] &&
            rx[15] == marker[1] && rx[16] == marker[2] && rx[17] == marker[3];
    }
    if (!got) {
      for (volatile uint32_t i = 0u; i < 100000u; i++) {
      }
    }
  }

  /* Put the PHY back to negotiating before judging, so a failure here does not
   * also leave the board unable to link on the next run. */
  (void)hal_eth_phy_write(ETH_PHY_BCR, ETH_PHY_BCR_RESET);
  for (volatile uint32_t i = 0u; i < 400000u; i++) {
  }
  (void)hal_eth_phy_write(ETH_PHY_BCR,
                          ETH_PHY_BCR_AUTONEG_EN | ETH_PHY_BCR_RESTART_AUTONEG);

  navtest_write(got ? "[eth loopback rx=ok]\r\n" : "[eth loopback rx=NONE]\r\n");
  TEST_ASSERT_TRUE(got);
}

NAVTEST_CASE_DECL(test_eth_phy_id_readable);
NAVTEST_CASE_DECL(test_eth_link_and_send);
NAVTEST_CASE_DECL(test_eth_phy_loopback_round_trips);

static const navtest_case_t eth_cases[] = {
    NAVTEST_CASE(test_eth_rejects_null_args),
    NAVTEST_CASE(test_eth_phy_id_readable),
    NAVTEST_CASE(test_eth_link_and_send),
    NAVTEST_CASE(test_eth_phy_loopback_round_trips),
};

const navtest_suite_t test_eth_suite = {
    .name = "ETH (cap)",
    .cases = eth_cases,
    .count = sizeof(eth_cases) / sizeof(eth_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_ETH */
