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
 * @file usb_reg.h
 * @brief USB OTG_FS peripheral register map for STM32F4 (device mode).
 *
 * @details
 * The Synopsys DWC2 core exposes four register blocks at fixed offsets from
 * the peripheral base: global (0x000), device (0x800), per-endpoint
 * (IN 0x900, OUT 0xB00, 0x20 stride) and the data FIFOs (0x1000, 0x1000
 * stride). Host-mode registers (0x400) are omitted — NavHAL drives the core
 * as a device only.
 *
 * Reference: RM0368 §22 (USB on-the-go full-speed).
 */

#ifndef CORTEX_M4_USB_REG_H
#define CORTEX_M4_USB_REG_H

#include "common/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_OTG_FS_BASE 0x50000000UL

/** @brief Core global register block (offset 0x000). */
typedef struct {
  __IO uint32_t GOTGCTL;    /**< 0x000: OTG control and status */
  __IO uint32_t GOTGINT;    /**< 0x004: OTG interrupt */
  __IO uint32_t GAHBCFG;    /**< 0x008: AHB configuration */
  __IO uint32_t GUSBCFG;    /**< 0x00C: USB configuration */
  __IO uint32_t GRSTCTL;    /**< 0x010: Reset */
  __IO uint32_t GINTSTS;    /**< 0x014: Core interrupt */
  __IO uint32_t GINTMSK;    /**< 0x018: Interrupt mask */
  __IO uint32_t GRXSTSR;    /**< 0x01C: Receive status debug read */
  __IO uint32_t GRXSTSP;    /**< 0x020: Receive status read and pop */
  __IO uint32_t GRXFSIZ;    /**< 0x024: Receive FIFO size */
  __IO uint32_t DIEPTXF0;   /**< 0x028: EP0 transmit FIFO size */
  __IO uint32_t HNPTXSTS;   /**< 0x02C: Non-periodic TX FIFO/queue status */
  uint32_t RESERVED0[2];    /**< 0x030-0x034 */
  __IO uint32_t GCCFG;      /**< 0x038: General core configuration */
  __IO uint32_t CID;        /**< 0x03C: Core ID */
  uint32_t RESERVED1[48];   /**< 0x040-0x0FC */
  __IO uint32_t HPTXFSIZ;   /**< 0x100: Host periodic TX FIFO size */
  __IO uint32_t DIEPTXF[3]; /**< 0x104-0x10C: Device IN EP1..3 TX FIFO size */
} USB_OTG_Global_Typedef;

/** @brief Device-mode register block (offset 0x800). */
typedef struct {
  __IO uint32_t DCFG;        /**< 0x800: Device configuration */
  __IO uint32_t DCTL;        /**< 0x804: Device control */
  __IO uint32_t DSTS;        /**< 0x808: Device status */
  uint32_t RESERVED0;        /**< 0x80C */
  __IO uint32_t DIEPMSK;     /**< 0x810: Device IN endpoint common mask */
  __IO uint32_t DOEPMSK;     /**< 0x814: Device OUT endpoint common mask */
  __IO uint32_t DAINT;       /**< 0x818: Device all-endpoints interrupt */
  __IO uint32_t DAINTMSK;    /**< 0x81C: Device all-endpoints mask */
  uint32_t RESERVED1[2];     /**< 0x820-0x824 */
  __IO uint32_t DVBUSDIS;    /**< 0x828: VBUS discharge time */
  __IO uint32_t DVBUSPULSE;  /**< 0x82C: VBUS pulsing time */
  uint32_t RESERVED2;        /**< 0x830 */
  __IO uint32_t DIEPEMPMSK;  /**< 0x834: IN EP FIFO-empty interrupt mask */
} USB_OTG_Device_Typedef;

/** @brief One device IN endpoint (0x900 + 0x20 * n). */
typedef struct {
  __IO uint32_t CTL;    /**< 0x00: DIEPCTLn */
  uint32_t RESERVED0;   /**< 0x04 */
  __IO uint32_t INT;    /**< 0x08: DIEPINTn */
  uint32_t RESERVED1;   /**< 0x0C */
  __IO uint32_t TSIZ;   /**< 0x10: DIEPTSIZn */
  __IO uint32_t DMA;    /**< 0x14: unused (no DMA on OTG_FS) */
  __IO uint32_t TXFSTS; /**< 0x18: DTXFSTSn — TX FIFO space, in words */
  uint32_t RESERVED2;   /**< 0x1C */
} USB_OTG_INEndpoint_Typedef;

/** @brief One device OUT endpoint (0xB00 + 0x20 * n). */
typedef struct {
  __IO uint32_t CTL;  /**< 0x00: DOEPCTLn */
  uint32_t RESERVED0; /**< 0x04 */
  __IO uint32_t INT;  /**< 0x08: DOEPINTn */
  uint32_t RESERVED1; /**< 0x0C */
  __IO uint32_t TSIZ; /**< 0x10: DOEPTSIZn */
  uint32_t RESERVED2[3];
} USB_OTG_OUTEndpoint_Typedef;

#define USB_GLOBAL ((USB_OTG_Global_Typedef *)(USB_OTG_FS_BASE))
#define USB_DEVICE ((USB_OTG_Device_Typedef *)(USB_OTG_FS_BASE + 0x800UL))
#define USB_INEP(n)                                                            \
  (((USB_OTG_INEndpoint_Typedef *)(USB_OTG_FS_BASE + 0x900UL)) + (n))
#define USB_OUTEP(n)                                                           \
  (((USB_OTG_OUTEndpoint_Typedef *)(USB_OTG_FS_BASE + 0xB00UL)) + (n))
/** @brief Push/pop port for FIFO @p n (each FIFO gets its own 4 KB window). */
#define USB_FIFO(n)                                                            \
  (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0x1000UL + ((n) * 0x1000UL)))
/** @brief Power and clock gating control (0xE00). */
#define USB_PCGCCTL (*(volatile uint32_t *)(USB_OTG_FS_BASE + 0xE00UL))

/* GAHBCFG */
#define USB_GAHBCFG_GINT (1U << 0)  /**< Global interrupt enable */
#define USB_GAHBCFG_TXFELVL (1U << 7) /**< TX FIFO empty at completely-empty */

/* GUSBCFG */
#define USB_GUSBCFG_PHYSEL (1U << 6) /**< Select the embedded FS transceiver */
#define USB_GUSBCFG_TRDT(x) (((x) & 0xFU) << 10) /**< Turnaround time */
#define USB_GUSBCFG_FDMOD (1U << 30)             /**< Force device mode */

/* GRSTCTL */
#define USB_GRSTCTL_CSRST (1U << 0)   /**< Core soft reset */
#define USB_GRSTCTL_RXFFLSH (1U << 4) /**< RX FIFO flush */
#define USB_GRSTCTL_TXFFLSH (1U << 5) /**< TX FIFO flush */
#define USB_GRSTCTL_TXFNUM(x) (((x) & 0x1FU) << 6) /**< FIFO to flush (0x10 = all) */
#define USB_GRSTCTL_AHBIDL (1U << 31)              /**< AHB master idle */

/* GINTSTS / GINTMSK */
#define USB_GINT_MMIS (1U << 1)     /**< Mode mismatch */
#define USB_GINT_OTGINT (1U << 2)   /**< OTG interrupt */
#define USB_GINT_SOF (1U << 3)      /**< Start of frame */
#define USB_GINT_RXFLVL (1U << 4)   /**< RX FIFO non-empty */
#define USB_GINT_ESUSP (1U << 10)   /**< Early suspend */
#define USB_GINT_USBSUSP (1U << 11) /**< USB suspend */
#define USB_GINT_USBRST (1U << 12)  /**< USB reset */
#define USB_GINT_ENUMDNE (1U << 13) /**< Enumeration done */
#define USB_GINT_IEPINT (1U << 18)  /**< IN endpoint interrupt */
#define USB_GINT_OEPINT (1U << 19)  /**< OUT endpoint interrupt */
#define USB_GINT_WKUPINT (1U << 31) /**< Resume / remote wakeup */

/* GRXSTSP (device mode) */
#define USB_GRXSTS_EPNUM(sts) ((sts) & 0xFU)
#define USB_GRXSTS_BCNT(sts) (((sts) >> 4) & 0x7FFU)
#define USB_GRXSTS_PKTSTS(sts) (((sts) >> 17) & 0xFU)
#define USB_PKTSTS_OUT_NAK 1U       /**< Global OUT NAK (no data) */
#define USB_PKTSTS_OUT_DATA 2U      /**< OUT data packet received */
#define USB_PKTSTS_OUT_COMPLETE 3U  /**< OUT transfer completed */
#define USB_PKTSTS_SETUP_COMPLETE 4U /**< SETUP stage completed */
#define USB_PKTSTS_SETUP_DATA 6U    /**< SETUP data packet received */

/* GCCFG */
#define USB_GCCFG_PWRDWN (1U << 16)     /**< Transceiver powered up */
#define USB_GCCFG_VBUSASEN (1U << 18)   /**< Enable VBUS sensing, A device */
#define USB_GCCFG_VBUSBSEN (1U << 19)   /**< Enable VBUS sensing, B device */
#define USB_GCCFG_NOVBUSSENS (1U << 21) /**< VBUS sensing disabled */

/* DCFG */
#define USB_DCFG_DSPD_FS 0x3U          /**< Full speed on the internal PHY */
#define USB_DCFG_NZLSOHSK (1U << 2)    /**< STALL a non-zero-length status OUT */
#define USB_DCFG_DAD(x) (((x) & 0x7FU) << 4) /**< Device address */
#define USB_DCFG_DAD_MASK (0x7FU << 4)

/* DCTL */
#define USB_DCTL_RWUSIG (1U << 0) /**< Remote wakeup signalling */
#define USB_DCTL_SDIS (1U << 1)   /**< Soft disconnect */
#define USB_DCTL_CGINAK (1U << 8) /**< Clear global IN NAK */

/* DSTS */
#define USB_DSTS_SUSPSTS (1U << 0)

/* DIEPMSK / DOEPMSK / DIEPINT / DOEPINT */
#define USB_EPINT_XFRC (1U << 0)  /**< Transfer completed */
#define USB_EPINT_EPDISD (1U << 1) /**< Endpoint disabled */
#define USB_DOEPINT_STUP (1U << 3) /**< SETUP phase done (OUT endpoints) */
#define USB_DIEPINT_TOC (1U << 3)  /**< Timeout condition (IN endpoints) */
#define USB_DIEPINT_TXFE (1U << 7) /**< TX FIFO empty */

/* DIEPCTL / DOEPCTL */
#define USB_EPCTL_MPSIZ(x) ((x) & 0x7FFU)
#define USB_EPCTL_USBAEP (1U << 15) /**< Endpoint active in this configuration */
#define USB_EPCTL_NAKSTS (1U << 17)
#define USB_EPCTL_EPTYP(x) (((x) & 0x3U) << 18)
#define USB_EPTYP_CONTROL 0U
#define USB_EPTYP_ISO 1U
#define USB_EPTYP_BULK 2U
#define USB_EPTYP_INTERRUPT 3U
#define USB_EPCTL_STALL (1U << 21)
#define USB_EPCTL_TXFNUM(x) (((x) & 0xFU) << 22)
#define USB_EPCTL_CNAK (1U << 26)
#define USB_EPCTL_SNAK (1U << 27)
#define USB_EPCTL_SD0PID (1U << 28) /**< Set DATA0 PID */
#define USB_EPCTL_EPDIS (1U << 30)
#define USB_EPCTL_EPENA (1U << 31)

/* DIEPTSIZ / DOEPTSIZ */
#define USB_EPTSIZ_XFRSIZ(x) ((x) & 0x7FFFFU)
#define USB_EPTSIZ_PKTCNT(x) (((x) & 0x3FFU) << 19)
#define USB_DOEPTSIZ_STUPCNT(x) (((x) & 0x3U) << 29)

/* DTXFSTS */
#define USB_DTXFSTS_AVAIL(x) ((x) & 0xFFFFU) /**< Free space, in 32-bit words */

/* FIFO size registers: low half-word is the start address (in words), high
 * half-word the depth (in words). */
#define USB_FIFOSIZE(start, depth) (((depth) << 16) | (start))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORTEX_M4_USB_REG_H */
