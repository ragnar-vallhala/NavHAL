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
 * @file eth_reg.h
 * @brief Ethernet MAC + DMA register map for STM32F7 (RM0410).
 *
 * @details
 * The MAC control block sits at 0x40028000 and the dedicated DMA block at
 * 0x40029000. This header also carries the RCC clock-enable bits and the
 * SYSCFG MII/RMII selection the driver needs to bring the peripheral up.
 */

#ifndef CORTEX_M7_ETH_REG_H
#define CORTEX_M7_ETH_REG_H

#include "common/hal_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Ethernet MAC control register block. */
typedef struct {
  __IO uint32_t MACCR;     /**< 0x000: MAC configuration. */
  __IO uint32_t MACFFR;    /**< 0x004: MAC frame filter. */
  __IO uint32_t MACHTHR;   /**< 0x008: Hash table high. */
  __IO uint32_t MACHTLR;   /**< 0x00C: Hash table low. */
  __IO uint32_t MACMIIAR;  /**< 0x010: MII address (MDIO). */
  __IO uint32_t MACMIIDR;  /**< 0x014: MII data (MDIO). */
  __IO uint32_t MACFCR;    /**< 0x018: Flow control. */
  __IO uint32_t MACVLANTR; /**< 0x01C: VLAN tag. */
  uint32_t RESERVED0[2];   /**< 0x020-0x024. */
  __IO uint32_t MACRWUFFR; /**< 0x028: Remote wake-up frame filter. */
  __IO uint32_t MACPMTCSR; /**< 0x02C: PMT control/status. */
  uint32_t RESERVED1;      /**< 0x030. */
  __IO uint32_t MACDBGR;   /**< 0x034: Debug. */
  __IO uint32_t MACSR;     /**< 0x038: Interrupt status. */
  __IO uint32_t MACIMR;    /**< 0x03C: Interrupt mask. */
  __IO uint32_t MACA0HR;   /**< 0x040: Address 0 high. */
  __IO uint32_t MACA0LR;   /**< 0x044: Address 0 low. */
  __IO uint32_t MACA1HR;   /**< 0x048: Address 1 high. */
  __IO uint32_t MACA1LR;   /**< 0x04C: Address 1 low. */
  __IO uint32_t MACA2HR;   /**< 0x050: Address 2 high. */
  __IO uint32_t MACA2LR;   /**< 0x054: Address 2 low. */
  __IO uint32_t MACA3HR;   /**< 0x058: Address 3 high. */
  __IO uint32_t MACA3LR;   /**< 0x05C: Address 3 low. */
} ETH_MAC_Typedef;

/** @brief Ethernet DMA register block. */
typedef struct {
  __IO uint32_t DMABMR;    /**< 0x000: Bus mode. */
  __IO uint32_t DMATPDR;   /**< 0x004: Transmit poll demand. */
  __IO uint32_t DMARPDR;   /**< 0x008: Receive poll demand. */
  __IO uint32_t DMARDLAR;  /**< 0x00C: Receive descriptor list address. */
  __IO uint32_t DMATDLAR;  /**< 0x010: Transmit descriptor list address. */
  __IO uint32_t DMASR;     /**< 0x014: Status. */
  __IO uint32_t DMAOMR;    /**< 0x018: Operation mode. */
  __IO uint32_t DMAIER;    /**< 0x01C: Interrupt enable. */
  __IO uint32_t DMAMFBOCR; /**< 0x020: Missed-frame / buffer-overflow counter. */
  __IO uint32_t DMARSWTR;  /**< 0x024: Receive status watchdog timer. */
  uint32_t RESERVED[8];    /**< 0x028-0x044. */
  __IO uint32_t DMACHTDR;  /**< 0x048: Current host transmit descriptor. */
  __IO uint32_t DMACHRDR;  /**< 0x04C: Current host receive descriptor. */
  __IO uint32_t DMACHTBAR; /**< 0x050: Current host transmit buffer address. */
  __IO uint32_t DMACHRBAR; /**< 0x054: Current host receive buffer address. */
} ETH_DMA_Typedef;

/** @brief MAC control block base address. */
#define ETH_MAC_BASE 0x40028000U
/** @brief DMA control block base address. */
#define ETH_DMA_BASE 0x40029000U

/** @brief MAC register block. */
#define ETH_MAC ((ETH_MAC_Typedef *)ETH_MAC_BASE)
/** @brief DMA register block. */
#define ETH_DMA ((ETH_DMA_Typedef *)ETH_DMA_BASE)

/* ---- MACCR (MAC configuration) ---- */
#define ETH_MACCR_RE (1U << 2)    /**< Receiver enable. */
#define ETH_MACCR_TE (1U << 3)    /**< Transmitter enable. */
#define ETH_MACCR_APCS (1U << 7)  /**< Automatic pad / CRC stripping. */
#define ETH_MACCR_IPCO (1U << 10) /**< IPv4 checksum offload. */
#define ETH_MACCR_DM (1U << 11)   /**< Duplex mode (1 = full). */
#define ETH_MACCR_LM (1U << 12)   /**< Loopback mode. */
#define ETH_MACCR_FES (1U << 14)  /**< Fast Ethernet speed (1 = 100M). */

/* ---- MACFFR (frame filter) ---- */
#define ETH_MACFFR_PM (1U << 0)  /**< Promiscuous mode. */
#define ETH_MACFFR_RA (1U << 31) /**< Receive all. */

/* ---- MACMIIAR (MDIO address) ---- */
#define ETH_MACMIIAR_MB (1U << 0)     /**< MII busy. */
#define ETH_MACMIIAR_MW (1U << 1)     /**< MII write. */
#define ETH_MACMIIAR_CR_SHIFT 2U      /**< Clock range field position. */
#define ETH_MACMIIAR_MR_SHIFT 6U      /**< MII register field position. */
#define ETH_MACMIIAR_PA_SHIFT 11U     /**< PHY address field position. */
#define ETH_MACMIIDR_MD_MASK 0xFFFFU  /**< MDIO data field. */

/* ---- DMABMR (bus mode) ---- */
#define ETH_DMABMR_SR (1U << 0)    /**< Software reset. */
#define ETH_DMABMR_DA (1U << 1)    /**< DMA arbitration. */
#define ETH_DMABMR_EDE (1U << 7)   /**< Enhanced descriptor format (unused). */
#define ETH_DMABMR_PBL_SHIFT 8U    /**< Programmable burst length position. */
#define ETH_DMABMR_FB (1U << 16)   /**< Fixed burst. */
#define ETH_DMABMR_AAB (1U << 25)  /**< Address-aligned beats. */

/* ---- DMASR (status) ---- */
#define ETH_DMASR_TS (1U << 0)    /**< Transmit status. */
#define ETH_DMASR_RS (1U << 6)    /**< Receive status. */
#define ETH_DMASR_RBUS (1U << 7)  /**< Receive buffer unavailable. */
#define ETH_DMASR_TBUS (1U << 2)  /**< Transmit buffer unavailable. */
#define ETH_DMASR_AIS (1U << 15)  /**< Abnormal interrupt summary. */
#define ETH_DMASR_NIS (1U << 16)  /**< Normal interrupt summary. */

/* ---- DMAOMR (operation mode) ---- */
#define ETH_DMAOMR_SR (1U << 1)   /**< Start/stop receive. */
#define ETH_DMAOMR_FTF (1U << 20) /**< Flush transmit FIFO. */
#define ETH_DMAOMR_TSF (1U << 21) /**< Transmit store-and-forward. */
#define ETH_DMAOMR_ST (1U << 13)  /**< Start/stop transmit. */
#define ETH_DMAOMR_RSF (1U << 25) /**< Receive store-and-forward. */

/* ---- DMAIER (interrupt enable) ---- */
#define ETH_DMAIER_TIE (1U << 0)   /**< Transmit interrupt enable. */
#define ETH_DMAIER_RIE (1U << 6)   /**< Receive interrupt enable. */
#define ETH_DMAIER_AISE (1U << 15) /**< Abnormal interrupt summary enable. */
#define ETH_DMAIER_NISE (1U << 16) /**< Normal interrupt summary enable. */

/* ---- RCC / SYSCFG (clocking + interface select) ---- */
#define ETH_AHB1ENR_ETHMACEN (1U << 25)   /**< ETH MAC clock enable. */
#define ETH_AHB1ENR_ETHMACTXEN (1U << 26) /**< ETH MAC TX clock enable. */
#define ETH_AHB1ENR_ETHMACRXEN (1U << 27) /**< ETH MAC RX clock enable. */
#define ETH_AHB1RSTR_ETHMACRST (1U << 25) /**< ETH MAC reset. */
#define ETH_APB2ENR_SYSCFGEN (1U << 14)   /**< SYSCFG clock enable. */

/** @brief SYSCFG peripheral-mode configuration register. */
#define ETH_SYSCFG_PMC (*(volatile uint32_t *)0x40013804U)
/** @brief 1 = RMII, 0 = MII interface select (SYSCFG_PMC bit 23). */
#define ETH_SYSCFG_PMC_MII_RMII_SEL (1U << 23)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M7_ETH_REG_H */
