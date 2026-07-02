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
 * @file eth_f7.c
 * @brief HAL Ethernet MAC driver for STM32F7 (Cortex-M7).
 *
 * @details
 * Frame-level media access over the F7 MAC and its dedicated DMA, driving an
 * external RMII/MII PHY over MDIO. The public API is in @c common/hal_eth.h; the
 * descriptor layout, ring sizing, and clause-22 PHY register set are in
 * @c navhal_port_eth.h; the register map is in @c family/eth_reg.h.
 *
 * The descriptors run in chained mode (each points to the next) with one buffer
 * per descriptor sized to hold a full frame, so every frame occupies a single
 * descriptor — no scatter/gather reassembly. The rings and buffers are tagged
 * ::NAVHAL_ETH_RAM so they land in DMA-reachable SRAM (the MAC DMA cannot reach
 * the CPU-local DTCM). They are coherent while the L1 D-cache is off (the
 * bring-up default); enabling it will require clean/invalidate around them.
 */

#include "common/hal_eth.h"

#if NAVHAL_CONFIG_DRV_ETH

#include "common/hal_clock.h"
#include "family/eth_reg.h"
#include "family/interrupt_reg.h"
#include "family/rcc_reg.h"
#include "navhal_port_eth.h"
#include "navhal_port_gpio.h"
#include "navhal_port_interrupt.h"

#define ETH_SPIN 1000000U /* bounded wait iterations for MDIO / DMA reset */

/* GPIO alternate function for the RMII/MII signals. */
#define GPIO_FUNC_ETH HAL_GPIO_AF11

/* Descriptor rings and per-descriptor buffers, in DMA-reachable SRAM. */
static NAVHAL_ETH_RAM navhal_eth_dma_desc_t _rx_desc[NAVHAL_ETH_RX_DESC_COUNT];
static NAVHAL_ETH_RAM navhal_eth_dma_desc_t _tx_desc[NAVHAL_ETH_TX_DESC_COUNT];
static NAVHAL_ETH_RAM uint8_t _rx_buf[NAVHAL_ETH_RX_DESC_COUNT][NAVHAL_ETH_BUF_SIZE];
static NAVHAL_ETH_RAM uint8_t _tx_buf[NAVHAL_ETH_TX_DESC_COUNT][NAVHAL_ETH_BUF_SIZE];

static uint32_t _rx_idx;      /* next RX descriptor the CPU will inspect */
static uint32_t _tx_idx;      /* next TX descriptor the CPU will fill */
static uint32_t _mii_cr;      /* MACMIIAR clock-range field for MDIO */
static uint8_t _phy_addr;     /* PHY address on the MDIO bus */
static uint8_t _mac[HAL_ETH_MAC_ADDR_LEN];
static bool _auto_neg;
static hal_eth_speed_t _speed;
static hal_eth_duplex_t _duplex;
static hal_eth_callback_t _cb;
static uint8_t _initialized;
static uint8_t _started;

/* -------------------------------------------------------------------------- */

/** @brief Copy @p n bytes (no libc dependency). Uses 32-bit accesses when both
 *  ends are word-aligned — the frame buffers are, and this roughly quarters the
 *  per-frame copy cost that otherwise caps throughput. */
static void _copy(uint8_t *dst, const uint8_t *src, uint32_t n) {
  if (((((uintptr_t)dst) | ((uintptr_t)src)) & 3u) == 0u) {
    uint32_t *d = (uint32_t *)(void *)dst;
    const uint32_t *s = (const uint32_t *)(const void *)src;
    uint32_t words = n >> 2;
    for (uint32_t i = 0; i < words; i++)
      d[i] = s[i];
    for (uint32_t i = words << 2; i < n; i++)
      dst[i] = src[i];
  } else {
    for (uint32_t i = 0; i < n; i++)
      dst[i] = src[i];
  }
}

/** @brief MACMIIAR clock-range field for the current HCLK (RM0410 MACMIIAR). */
static uint32_t _mdio_clock_range(uint32_t hclk_hz) {
  uint32_t mhz = hclk_hz / 1000000U;
  if (mhz >= 150U)
    return 0x4U; /* 150-216 MHz -> HCLK/102 */
  if (mhz >= 100U)
    return 0x1U; /* 100-150 MHz -> HCLK/62 */
  if (mhz >= 60U)
    return 0x0U; /* 60-100 MHz -> HCLK/42 */
  if (mhz >= 35U)
    return 0x3U; /* 35-60 MHz -> HCLK/26 */
  return 0x2U;   /* 20-35 MHz -> HCLK/16 */
}

static void _cfg_pin(hal_gpio_pin_t pin) {
  hal_gpio_enable_clock(pin);
  hal_gpio_set_mode(pin, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
  hal_gpio_set_alternate_function(pin, GPIO_FUNC_ETH);
  hal_gpio_set_output_type(pin, HAL_GPIO_OTYPE_PUSH_PULL);
  hal_gpio_set_output_speed(pin, HAL_GPIO_SPEED_VERY_HIGH);
}

/* RMII pinout on the Nucleo-F767ZI: REF_CLK PA1, MDIO PA2, MDC PC1, CRS_DV PA7,
 * RXD0 PC4, RXD1 PC5, TX_EN PG11, TXD0 PG13, TXD1 PB13 (all AF11). TXD1 is on
 * PB13, not the MCU's alternative PG14 — this board routes it to PB13. MII adds
 * more signals and is not wired on this board. */
static void _cfg_gpio(hal_eth_phy_iface_t iface) {
  (void)iface; /* only RMII is wired on the supported board */
  _cfg_pin(GPIO_PA01);
  _cfg_pin(GPIO_PA02);
  _cfg_pin(GPIO_PA07);
  _cfg_pin(GPIO_PC01);
  _cfg_pin(GPIO_PC04);
  _cfg_pin(GPIO_PC05);
  _cfg_pin(GPIO_PG11);
  _cfg_pin(GPIO_PG13);
  _cfg_pin(GPIO_PB13);
}

/* -------------------------------------------------------------------------- */

hal_status_t hal_eth_phy_read(uint8_t reg, uint16_t *out) {
  if (out == NULL)
    return HAL_ERR_INVALID_ARG;
  uint32_t spin = ETH_SPIN;
  while ((ETH_MAC->MACMIIAR & ETH_MACMIIAR_MB) && spin--)
    ;
  if (!spin)
    return HAL_ERR_TIMEOUT;

  ETH_MAC->MACMIIAR = ((uint32_t)_phy_addr << ETH_MACMIIAR_PA_SHIFT) |
                      ((uint32_t)(reg & 0x1FU) << ETH_MACMIIAR_MR_SHIFT) |
                      (_mii_cr << ETH_MACMIIAR_CR_SHIFT) | ETH_MACMIIAR_MB;

  spin = ETH_SPIN;
  while ((ETH_MAC->MACMIIAR & ETH_MACMIIAR_MB) && spin--)
    ;
  if (!spin)
    return HAL_ERR_TIMEOUT;

  *out = (uint16_t)(ETH_MAC->MACMIIDR & ETH_MACMIIDR_MD_MASK);
  return HAL_OK;
}

hal_status_t hal_eth_phy_write(uint8_t reg, uint16_t val) {
  uint32_t spin = ETH_SPIN;
  while ((ETH_MAC->MACMIIAR & ETH_MACMIIAR_MB) && spin--)
    ;
  if (!spin)
    return HAL_ERR_TIMEOUT;

  ETH_MAC->MACMIIDR = val;
  ETH_MAC->MACMIIAR = ((uint32_t)_phy_addr << ETH_MACMIIAR_PA_SHIFT) |
                      ((uint32_t)(reg & 0x1FU) << ETH_MACMIIAR_MR_SHIFT) |
                      (_mii_cr << ETH_MACMIIAR_CR_SHIFT) | ETH_MACMIIAR_MW |
                      ETH_MACMIIAR_MB;

  spin = ETH_SPIN;
  while ((ETH_MAC->MACMIIAR & ETH_MACMIIAR_MB) && spin--)
    ;
  return spin ? HAL_OK : HAL_ERR_TIMEOUT;
}

/* Derive speed/duplex from the auto-negotiation result (ANAR & ANLPAR). */
static void _read_negotiated(hal_eth_speed_t *speed, hal_eth_duplex_t *duplex) {
  uint16_t adv = 0, lpa = 0;
  hal_eth_phy_read(ETH_PHY_ANAR, &adv);
  hal_eth_phy_read(ETH_PHY_ANLPAR, &lpa);
  uint16_t common = (uint16_t)(adv & lpa);
  if (common & ETH_PHY_ANEG_100_FD) {
    *speed = HAL_ETH_SPEED_100M;
    *duplex = HAL_ETH_FULL_DUPLEX;
  } else if (common & ETH_PHY_ANEG_100_HD) {
    *speed = HAL_ETH_SPEED_100M;
    *duplex = HAL_ETH_HALF_DUPLEX;
  } else if (common & ETH_PHY_ANEG_10_FD) {
    *speed = HAL_ETH_SPEED_10M;
    *duplex = HAL_ETH_FULL_DUPLEX;
  } else {
    *speed = HAL_ETH_SPEED_10M;
    *duplex = HAL_ETH_HALF_DUPLEX;
  }
}

/* Write the MAC's speed/duplex (MACCR FES/DM) from the cached link mode. */
static void _apply_speed_duplex(void) {
  uint32_t cr = ETH_MAC->MACCR & ~(ETH_MACCR_FES | ETH_MACCR_DM);
  if (_speed == HAL_ETH_SPEED_100M)
    cr |= ETH_MACCR_FES;
  if (_duplex == HAL_ETH_FULL_DUPLEX)
    cr |= ETH_MACCR_DM;
  ETH_MAC->MACCR = cr;
}

static void _init_rings(void) {
  for (uint32_t i = 0; i < NAVHAL_ETH_RX_DESC_COUNT; i++) {
    _rx_desc[i].des0 = ETH_DMA_DESC_OWN; /* hand to the DMA */
    _rx_desc[i].des1 =
        ETH_DMA_RDES1_RCH | (NAVHAL_ETH_BUF_SIZE & ETH_DMA_RDES1_RBS1_MASK);
    _rx_desc[i].des2 = (uint32_t)(uintptr_t)&_rx_buf[i][0];
    _rx_desc[i].des3 =
        (uint32_t)(uintptr_t)&_rx_desc[(i + 1U) % NAVHAL_ETH_RX_DESC_COUNT];
  }
  for (uint32_t i = 0; i < NAVHAL_ETH_TX_DESC_COUNT; i++) {
    _tx_desc[i].des0 = ETH_DMA_TDES0_TCH; /* chained; CPU owns (OWN clear) */
    _tx_desc[i].des1 = 0;
    _tx_desc[i].des2 = (uint32_t)(uintptr_t)&_tx_buf[i][0];
    _tx_desc[i].des3 =
        (uint32_t)(uintptr_t)&_tx_desc[(i + 1U) % NAVHAL_ETH_TX_DESC_COUNT];
  }
  _rx_idx = 0;
  _tx_idx = 0;
  ETH_DMA->DMARDLAR = (uint32_t)(uintptr_t)&_rx_desc[0];
  ETH_DMA->DMATDLAR = (uint32_t)(uintptr_t)&_tx_desc[0];
}

/* -------------------------------------------------------------------------- */

hal_status_t hal_eth_set_mac_address(const uint8_t mac[HAL_ETH_MAC_ADDR_LEN]) {
  if (mac == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint8_t i = 0; i < HAL_ETH_MAC_ADDR_LEN; i++)
    _mac[i] = mac[i];
  ETH_MAC->MACA0HR = ((uint32_t)mac[5] << 8) | (uint32_t)mac[4];
  ETH_MAC->MACA0LR = ((uint32_t)mac[3] << 24) | ((uint32_t)mac[2] << 16) |
                     ((uint32_t)mac[1] << 8) | (uint32_t)mac[0];
  return HAL_OK;
}

hal_status_t hal_eth_get_mac_address(uint8_t mac[HAL_ETH_MAC_ADDR_LEN]) {
  if (mac == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint8_t i = 0; i < HAL_ETH_MAC_ADDR_LEN; i++)
    mac[i] = _mac[i];
  return HAL_OK;
}

hal_status_t hal_eth_set_callback(hal_eth_callback_t callback) {
  _cb = callback;
  return HAL_OK;
}

hal_status_t hal_eth_init(const hal_eth_config_t *config) {
  if (config == NULL)
    return HAL_ERR_INVALID_ARG;
  if (_initialized)
    return HAL_ERR_NOT_INITIALIZED;

  /* SYSCFG selects MII vs RMII and must be set before the ETH clocks run. */
  RCC->APB2ENR |= ETH_APB2ENR_SYSCFGEN;
  if (config->interface == HAL_ETH_IFACE_RMII)
    ETH_SYSCFG_PMC |= ETH_SYSCFG_PMC_MII_RMII_SEL;
  else
    ETH_SYSCFG_PMC &= ~ETH_SYSCFG_PMC_MII_RMII_SEL;

  _cfg_gpio(config->interface);

  RCC->AHB1ENR |= ETH_AHB1ENR_ETHMACEN | ETH_AHB1ENR_ETHMACTXEN |
                  ETH_AHB1ENR_ETHMACRXEN;

  /* Reset the MAC DMA and wait for the self-clearing SR bit. */
  ETH_DMA->DMABMR |= ETH_DMABMR_SR;
  uint32_t spin = ETH_SPIN;
  while ((ETH_DMA->DMABMR & ETH_DMABMR_SR) && spin--)
    ;
  if (!spin)
    return HAL_ERR_TIMEOUT;

  _mii_cr = _mdio_clock_range(hal_clock_get_ahbclk());
  _phy_addr = config->phy_address;
  _auto_neg = config->auto_negotiation;

  /* Reset the PHY and wait for the self-clearing reset bit. */
  hal_status_t s = hal_eth_phy_write(ETH_PHY_BCR, ETH_PHY_BCR_RESET);
  if (s != HAL_OK)
    return s;
  spin = ETH_SPIN;
  uint16_t bcr = ETH_PHY_BCR_RESET;
  while (spin--) {
    if (hal_eth_phy_read(ETH_PHY_BCR, &bcr) == HAL_OK &&
        !(bcr & ETH_PHY_BCR_RESET))
      break;
  }

  if (_auto_neg) {
    hal_eth_phy_write(ETH_PHY_BCR,
                      ETH_PHY_BCR_AUTONEG_EN | ETH_PHY_BCR_RESTART_AUTONEG);
    /* Assume the near-universal 100M/full result and do not block on
     * negotiation, so a board with no cable still comes up promptly.
     * hal_eth_get_link resyncs the MAC to the actual link once it is up. */
    _speed = HAL_ETH_SPEED_100M;
    _duplex = HAL_ETH_FULL_DUPLEX;
  } else {
    _speed = config->speed;
    _duplex = config->duplex;
    hal_eth_phy_write(ETH_PHY_BCR,
                      (uint16_t)((_speed == HAL_ETH_SPEED_100M
                                      ? ETH_PHY_BCR_SPEED_100
                                      : 0) |
                                 (_duplex == HAL_ETH_FULL_DUPLEX
                                      ? ETH_PHY_BCR_FULLDUPLEX
                                      : 0)));
  }

  /* MAC speed/duplex. RE/TE are added by hal_eth_start so no traffic moves
   * until the caller is ready. */
  _apply_speed_duplex();
  ETH_MAC->MACFFR = config->promiscuous ? ETH_MACFFR_PM : 0;

  hal_eth_set_mac_address(config->mac_addr);

  _init_rings();

  ETH_DMA->DMAOMR = ETH_DMAOMR_TSF | ETH_DMAOMR_RSF; /* store-and-forward. */
  ETH_DMA->DMABMR |= ETH_DMABMR_FB | ETH_DMABMR_AAB | (32U << ETH_DMABMR_PBL_SHIFT);

  _initialized = 1;
  return HAL_OK;
}

static void _eth_irq_handler(void);

hal_status_t hal_eth_start(void) {
  if (!_initialized)
    return HAL_ERR_NOT_INITIALIZED;

  ETH_MAC->MACCR |= ETH_MACCR_RE | ETH_MACCR_TE;
  ETH_DMA->DMAOMR |= ETH_DMAOMR_FTF; /* flush TX FIFO before starting. */

  /* DMA interrupts are only enabled when a callback is registered to service
   * them (set the callback before start). Without one, a polling caller drains
   * via hal_eth_receive; enabling RX interrupts then would storm on
   * receive-buffer-unavailable once the ring fills, with nothing to drain it. */
  if (_cb) {
    ETH_DMA->DMAIER =
        ETH_DMAIER_NISE | ETH_DMAIER_AISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE;
    hal_interrupt_attach_callback((hal_irq_t)ETH_IRQn, _eth_irq_handler);
    hal_interrupt_enable_with_priority((hal_irq_t)ETH_IRQn,
                                       HAL_IRQ_PRIORITY_DEFAULT);
  } else {
    ETH_DMA->DMAIER = 0;
  }

  ETH_DMA->DMAOMR |= ETH_DMAOMR_ST | ETH_DMAOMR_SR;
  _started = 1;
  return HAL_OK;
}

hal_status_t hal_eth_stop(void) {
  ETH_DMA->DMAOMR &= ~(ETH_DMAOMR_ST | ETH_DMAOMR_SR);
  ETH_MAC->MACCR &= ~(ETH_MACCR_RE | ETH_MACCR_TE);
  hal_interrupt_disable((hal_irq_t)ETH_IRQn);
  _started = 0;
  return HAL_OK;
}

hal_status_t hal_eth_deinit(void) {
  hal_eth_stop();
  RCC->AHB1ENR &= ~(ETH_AHB1ENR_ETHMACEN | ETH_AHB1ENR_ETHMACTXEN |
                    ETH_AHB1ENR_ETHMACRXEN);
  _initialized = 0;
  return HAL_OK;
}

hal_status_t hal_eth_send(const uint8_t *frame, uint16_t len) {
  if (frame == NULL || len < HAL_ETH_MIN_FRAME_LEN || len > HAL_ETH_MAX_FRAME_LEN)
    return HAL_ERR_INVALID_ARG;
  if (!_started)
    return HAL_ERR_NOT_INITIALIZED;

  navhal_eth_dma_desc_t *d = &_tx_desc[_tx_idx];
  if (d->des0 & ETH_DMA_DESC_OWN)
    return HAL_ERR_BUSY; /* the DMA still owns this descriptor. */

  _copy(&_tx_buf[_tx_idx][0], frame, len);
  d->des1 = (uint32_t)len & ETH_DMA_TDES1_TBS1_MASK;
  d->des0 = ETH_DMA_TDES0_TCH | ETH_DMA_TDES0_FS | ETH_DMA_TDES0_LS |
            ETH_DMA_TDES0_IC | ETH_DMA_DESC_OWN;
  _tx_idx = (_tx_idx + 1U) % NAVHAL_ETH_TX_DESC_COUNT;

  /* Clear a pending "transmit buffer unavailable" and poke the DMA to fetch the
   * freshly-owned descriptor. */
  ETH_DMA->DMASR = ETH_DMASR_TBUS;
  ETH_DMA->DMATPDR = 0;
  return HAL_OK;
}

hal_status_t hal_eth_receive(uint8_t *buf, uint16_t max_len, uint16_t *out_len) {
  if (buf == NULL || out_len == NULL)
    return HAL_ERR_INVALID_ARG;
  if (!_started)
    return HAL_ERR_NOT_INITIALIZED;
  *out_len = 0;

  navhal_eth_dma_desc_t *d = &_rx_desc[_rx_idx];
  if (d->des0 & ETH_DMA_DESC_OWN)
    return HAL_OK; /* no completed frame. */

  hal_status_t rc = HAL_OK;
  if (!(d->des0 & ETH_DMA_RDES0_ES)) {
    /* FL counts the frame including the 4-byte FCS; hand back the frame without
     * it, matching hal_eth_send's contract. Buffers hold a full frame, so the
     * frame is always in this single descriptor. */
    uint32_t fl = ETH_DMA_RDES0_FRAME_LEN(d->des0);
    uint16_t len = (uint16_t)(fl >= 4U ? fl - 4U : fl);
    if (len > max_len) {
      rc = HAL_ERR_NO_MEM;
    } else {
      _copy(buf, &_rx_buf[_rx_idx][0], len);
      *out_len = len;
    }
  }

  d->des0 = ETH_DMA_DESC_OWN; /* return the descriptor to the DMA. */
  _rx_idx = (_rx_idx + 1U) % NAVHAL_ETH_RX_DESC_COUNT;

  /* Clear a pending "receive buffer unavailable" and resume reception. */
  ETH_DMA->DMASR = ETH_DMASR_RBUS;
  ETH_DMA->DMARPDR = 0;
  return rc;
}

bool hal_eth_link_is_up(void) {
  uint16_t bsr = 0;
  /* The link-status bit latches low; read twice for the current state. */
  hal_eth_phy_read(ETH_PHY_BSR, &bsr);
  hal_eth_phy_read(ETH_PHY_BSR, &bsr);
  return (bsr & ETH_PHY_BSR_LINK_UP) != 0;
}

hal_status_t hal_eth_get_link(hal_eth_link_t *out) {
  if (out == NULL)
    return HAL_ERR_INVALID_ARG;
  out->up = hal_eth_link_is_up();
  /* When auto-negotiating, refresh the cached mode from the PHY and resync the
   * MAC to it once the link is up (init did not block on negotiation). */
  if (_auto_neg && out->up) {
    _read_negotiated(&_speed, &_duplex);
    _apply_speed_duplex();
  }
  out->speed = _speed;
  out->duplex = _duplex;
  return HAL_OK;
}

/* Runs in ETH IRQ context; the registered callback does too. */
static void _eth_irq_handler(void) {
  uint32_t sr = ETH_DMA->DMASR;
  if ((sr & ETH_DMASR_RS) && _cb)
    _cb(HAL_ETH_EVENT_RX);
  if ((sr & ETH_DMASR_TS) && _cb)
    _cb(HAL_ETH_EVENT_TX_DONE);
  if ((sr & ETH_DMASR_AIS) && _cb)
    _cb(HAL_ETH_EVENT_ERROR);
  /* Clear every latched W1C status bit (bits 0..16), including the NIS/AIS
   * summaries and TBUS/RBUS. Clearing only TS/RS/NIS would leave TBUS set once
   * the TX ring drains, which keeps NIS asserted and storms the interrupt. */
  ETH_DMA->DMASR = sr & 0x0001FFFFU;
}

#endif /* NAVHAL_CONFIG_DRV_ETH */
