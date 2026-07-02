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
 * @file port/cortex-m7/navhal_port_eth.h
 * @brief Cortex-M7 / STM32F7 Ethernet port header.
 *
 * @details
 * The public Ethernet API lives in @c common/hal_eth.h. This header carries the
 * target-specific compile-time content the F7 driver builds against: the ETH DMA
 * descriptor layout and its status/control bits, the descriptor-ring sizing, and
 * the IEEE 802.3 clause-22 PHY register set used over MDIO. The MAC/DMA register
 * map (base addresses and peripheral register bits) is family-private and lands
 * with the driver in @c family/eth_reg.h; it is intentionally not included here
 * so this header is self-contained.
 *
 * The whole body is compiled only when @c NAVHAL_CONFIG_DRV_ETH is set.
 *
 * The descriptors use the "normal" (4-word) format — the reset default with the
 * enhanced descriptor format disabled. The enhanced 8-word format (required for
 * IEEE-1588 timestamping) is a later addition.
 *
 * @note The descriptor rings and frame buffers are accessed by the MAC DMA,
 *       which reaches SRAM but not the CPU-tightly-coupled DTCM. Tag them with
 *       ::NAVHAL_ETH_RAM so the linker places them in the dedicated ETHRAM
 *       region (SRAM). The buffer size and alignment below put buffers on
 *       32-byte cache-line boundaries so they can be cleaned/invalidated once
 *       the L1 D-cache is enabled (it is off at bring-up, so they are coherent).
 */

#ifndef NAVHAL_PORT_ETH_H
#define NAVHAL_PORT_ETH_H

#include "common/hal_eth.h"
#include "common/hal_types.h"


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_ETH

#include <stdint.h>

/* ------------------------------------------------------------------ *
 * Descriptor ring sizing
 * ------------------------------------------------------------------ */

/** @brief Number of RX descriptors / buffers in the ring. */
#define NAVHAL_ETH_RX_DESC_COUNT 4U

/** @brief Number of TX descriptors / buffers in the ring. */
#define NAVHAL_ETH_TX_DESC_COUNT 4U

/** @brief Per-descriptor buffer size: >= ::HAL_ETH_MAX_FRAME_LEN, rounded up to
 *         a 32-byte cache line so buffers stay individually cache-maintainable. */
#define NAVHAL_ETH_BUF_SIZE 1536U

/** @brief Required alignment (bytes) for descriptor arrays and buffers. */
#define NAVHAL_ETH_MEM_ALIGN 32U

/** @brief Placement for the DMA descriptor rings and frame buffers: the ETHRAM
 *         linker region (DMA-reachable SRAM), aligned to a cache line. */
#define NAVHAL_ETH_RAM                                                          \
  __attribute__((section(".eth_ram"), aligned(NAVHAL_ETH_MEM_ALIGN)))

/* ------------------------------------------------------------------ *
 * DMA descriptor (normal 4-word format, STM32F7 ETH)
 * ------------------------------------------------------------------ */

/**
 * @brief One MAC DMA descriptor.
 *
 * Field meaning depends on direction (see the TDESn / RDESn bit macros): des0
 * is status + OWN, des1 is control + buffer size, des2 is the buffer address,
 * and des3 is the second-buffer or (when chained) next-descriptor address.
 */
typedef struct {
  __IO uint32_t des0; /**< Status word; bit 31 is OWN. */
  __IO uint32_t des1; /**< Control + buffer-1 byte count. */
  __IO uint32_t des2; /**< Buffer-1 address. */
  __IO uint32_t des3; /**< Buffer-2 / next-descriptor address (chained). */
} navhal_eth_dma_desc_t;

/** @brief OWN bit (des0): set = the DMA owns the descriptor, clear = the CPU. */
#define ETH_DMA_DESC_OWN (1U << 31)

/* --- TX descriptor (TDES0/1) --- */
#define ETH_DMA_TDES0_IC (1U << 30)  /**< Interrupt on completion. */
#define ETH_DMA_TDES0_LS (1U << 29)  /**< Last segment of the frame. */
#define ETH_DMA_TDES0_FS (1U << 28)  /**< First segment of the frame. */
#define ETH_DMA_TDES0_TER (1U << 21) /**< Transmit end of ring. */
#define ETH_DMA_TDES0_TCH (1U << 20) /**< Second address chained. */
#define ETH_DMA_TDES0_ES (1U << 15)  /**< Error summary. */
#define ETH_DMA_TDES1_TBS1_MASK 0x1FFFU /**< Buffer-1 byte count. */

/* --- RX descriptor (RDES0/1) --- */
#define ETH_DMA_RDES0_ES (1U << 15)      /**< Error summary. */
#define ETH_DMA_RDES0_FS (1U << 9)       /**< First descriptor of the frame. */
#define ETH_DMA_RDES0_LS (1U << 8)       /**< Last descriptor of the frame. */
#define ETH_DMA_RDES0_FL_SHIFT 16U       /**< Frame length field position. */
#define ETH_DMA_RDES0_FL_MASK 0x3FFFU    /**< Frame length field width. */
#define ETH_DMA_RDES1_RER (1U << 15)     /**< Receive end of ring. */
#define ETH_DMA_RDES1_RCH (1U << 14)     /**< Second address chained. */
#define ETH_DMA_RDES1_RBS1_MASK 0x1FFFU  /**< Buffer-1 byte count. */

/** @brief Extract the received frame length (bytes) from an RX des0. */
#define ETH_DMA_RDES0_FRAME_LEN(des0)                                          \
  (((des0) >> ETH_DMA_RDES0_FL_SHIFT) & ETH_DMA_RDES0_FL_MASK)

/* ------------------------------------------------------------------ *
 * PHY registers (IEEE 802.3 clause 22 — PHY-agnostic, via MDIO)
 * ------------------------------------------------------------------ */

#define ETH_PHY_BCR 0x00U    /**< Basic Control Register. */
#define ETH_PHY_BSR 0x01U    /**< Basic Status Register. */
#define ETH_PHY_ID1 0x02U    /**< PHY Identifier 1. */
#define ETH_PHY_ID2 0x03U    /**< PHY Identifier 2. */
#define ETH_PHY_ANAR 0x04U   /**< Auto-Negotiation Advertisement. */
#define ETH_PHY_ANLPAR 0x05U /**< Auto-Negotiation Link Partner Ability. */

/* Basic Control Register (BCR) bits. */
#define ETH_PHY_BCR_RESET (1U << 15)          /**< Software reset. */
#define ETH_PHY_BCR_LOOPBACK (1U << 14)       /**< Enable loopback. */
#define ETH_PHY_BCR_SPEED_100 (1U << 13)      /**< 1 = 100M, 0 = 10M. */
#define ETH_PHY_BCR_AUTONEG_EN (1U << 12)     /**< Enable auto-negotiation. */
#define ETH_PHY_BCR_POWERDOWN (1U << 11)      /**< Power down. */
#define ETH_PHY_BCR_RESTART_AUTONEG (1U << 9) /**< Restart auto-negotiation. */
#define ETH_PHY_BCR_FULLDUPLEX (1U << 8)      /**< 1 = full, 0 = half duplex. */

/* Basic Status Register (BSR) bits. */
#define ETH_PHY_BSR_AUTONEG_DONE (1U << 5) /**< Auto-negotiation complete. */
#define ETH_PHY_BSR_LINK_UP (1U << 2)      /**< Link is up. */

/* Auto-negotiation ability bits (shared by ANAR / ANLPAR). */
#define ETH_PHY_ANEG_100_FD (1U << 8) /**< 100BASE-TX full duplex. */
#define ETH_PHY_ANEG_100_HD (1U << 7) /**< 100BASE-TX half duplex. */
#define ETH_PHY_ANEG_10_FD (1U << 6)  /**< 10BASE-T full duplex. */
#define ETH_PHY_ANEG_10_HD (1U << 5)  /**< 10BASE-T half duplex. */

#endif /* NAVHAL_CONFIG_DRV_ETH */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_ETH_H */
