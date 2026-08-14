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
 * @file usb_cdc.c
 * @brief USB CDC-ACM device driver for the STM32F4 OTG_FS core.
 *
 * @details
 * A single-configuration full-speed device: control endpoint 0, bulk IN/OUT
 * endpoint 1 carrying the serial data, and interrupt IN endpoint 2 carrying
 * CDC serial-state notifications. The core has no DMA, so packets are copied
 * word-wise through the FIFO push/pop windows.
 *
 * Everything after ::hal_usb_cdc_init runs from the OTG_FS interrupt:
 * enumeration, control transfers, and moving received bytes into the RX ring
 * buffer. Only ::hal_usb_cdc_write and ::hal_usb_cdc_notify_serial_state touch
 * the hardware from thread context, and only after the interrupt has reported
 * the previous transfer on that endpoint done — so neither may be called from
 * an interrupt handler, including the RX callback.
 *
 * Reference: RM0368 §22 (device programming model), USB 2.0 §9, and
 * USB CDC 1.2 / PSTN 1.2 for the ACM subclass.
 */

#include "common/hal_usb_cdc.h"

#if NAVHAL_CONFIG_DRV_USB_CDC

#include "board.h"
#include "family/rcc_reg.h"
#include "family/usb_reg.h"
#include "navhal_port_gpio.h"
#include "navhal_port_interrupt.h"
#include <stdint.h>

#ifndef BOARD_HSE_FREQ_HZ
#define BOARD_HSE_FREQ_HZ 8000000U
#endif
#ifndef BOARD_HSI_FREQ_HZ
#define BOARD_HSI_FREQ_HZ 16000000U
#endif

/* Endpoint plan. The FIFO numbers match the IN endpoint numbers, which is what
 * DIEPCTL.TXFNUM is programmed with. */
#define EP_CTRL 0U
#define EP_DATA 1U /* 0x81 bulk IN + 0x01 bulk OUT — the serial stream */
#define EP_NOTIF 2U /* 0x82 interrupt IN — CDC serial-state notifications */

#define EP0_MAX_PACKET 64U
/* 16, not the more common 8: a SERIAL_STATE notification is 10 bytes, and a
 * packet size that splits it in two buys nothing but a second transaction. */
#define NOTIF_MAX_PACKET 16U

/* FIFO RAM budget: the OTG_FS core has 1.25 KB = 320 words, split here as
 * 128 (shared RX) + 64 (EP0 TX) + 96 (bulk TX) + 32 (notification TX). */
#define FIFO_RX_WORDS 128U
#define FIFO_TX0_WORDS 64U
#define FIFO_TX1_WORDS 96U
#define FIFO_TX2_WORDS 32U

/* Largest slice handed to the core in one IN transfer. Bounded by the 384-byte
 * bulk TX FIFO, since the whole slice is pushed in one go. */
#define TX_CHUNK 256U

/* Bounded spin waiting for the host to collect an IN transfer. The host polls a
 * bulk endpoint every frame when the port is open, so this only ever expires
 * when the far end stopped reading. */
#define TX_TIMEOUT_SPINS 2000000UL

/* Bounded spin for core reset / mode-change settling (RM0368 §22.17.1). */
#define CORE_TIMEOUT_SPINS 200000UL

#define RX_RING_SIZE 512U /* power of two */

/* USB 2.0 §9.4 standard requests */
#define REQ_GET_STATUS 0x00
#define REQ_CLEAR_FEATURE 0x01
#define REQ_SET_FEATURE 0x03
#define REQ_SET_ADDRESS 0x05
#define REQ_GET_DESCRIPTOR 0x06
#define REQ_GET_CONFIGURATION 0x08
#define REQ_SET_CONFIGURATION 0x09
#define REQ_GET_INTERFACE 0x0A
#define REQ_SET_INTERFACE 0x0B

/* CDC PSTN 1.2 §6.3 class requests */
#define CDC_SET_LINE_CODING 0x20
#define CDC_GET_LINE_CODING 0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22
#define CDC_SEND_BREAK 0x23

/* -------------------------------------------------------------------------- *
 * Descriptors
 * -------------------------------------------------------------------------- */

/* ST's VID/PID for a virtual COM port. The host binds by class (cdc_acm on
 * Linux, usbser.sys on Windows), so this only decides how the port is named. */
#define USB_VID 0x0483
#define USB_PID 0x5740

static const uint8_t device_desc[18] = {
    18,   0x01,       /* bLength, DEVICE */
    0x00, 0x02,       /* bcdUSB 2.00 */
    0x02, 0x00, 0x00, /* class CDC, no subclass/protocol at device level */
    EP0_MAX_PACKET, USB_VID & 0xFF, USB_VID >> 8, USB_PID & 0xFF, USB_PID >> 8,
    0x00, 0x02,      /* bcdDevice 2.00 */
    0x01, 0x02, 0x03, /* iManufacturer, iProduct, iSerialNumber */
    0x01,            /* bNumConfigurations */
};

static const uint8_t config_desc[67] = {
    /* Configuration: 2 interfaces, bus powered, 100 mA */
    9, 0x02, 67, 0x00, 0x02, 0x01, 0x00, 0x80, 50,

    /* Interface 0 — CDC communication, ACM subclass, AT-command protocol */
    9, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* Header functional descriptor, CDC 1.10 */
    5, 0x24, 0x00, 0x10, 0x01,
    /* Call management: device does not handle call management */
    5, 0x24, 0x01, 0x00, 0x01,
    /* ACM functional. bmCapabilities bit1: Set/Get_Line_Coding,
     * Set_Control_Line_State and the SerialState notification; bit2: Send_Break.
     * A host will not issue a request this byte does not claim — Linux quietly
     * fails tcsendbreak() without bit2 set. */
    4, 0x24, 0x02, 0x06,
    /* Union functional: control interface 0, subordinate data interface 1 */
    5, 0x24, 0x06, 0x00, 0x01,
    /* Notification endpoint 0x82, interrupt, 16 ms polling */
    7, 0x05, 0x82, 0x03, NOTIF_MAX_PACKET, 0x00, 0x10,

    /* Interface 1 — CDC data */
    9, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk OUT 0x01, 64 bytes */
    7, 0x05, 0x01, 0x02, HAL_USB_CDC_PACKET_SIZE, 0x00, 0x00,
    /* Bulk IN 0x81, 64 bytes */
    7, 0x05, 0x81, 0x02, HAL_USB_CDC_PACKET_SIZE, 0x00, 0x00,
};

static const uint8_t string_langid[4] = {4, 0x03, 0x09, 0x04}; /* en-US */
static const char *const string_table[] = {
    "NAVRobotec",     /* iManufacturer */
    "NavHAL USB CDC", /* iProduct */
    /* Fixed serial: one board per host is the assumption. Two boards with the
     * same serial still enumerate, they just get unstable /dev/serial/by-id
     * names. ponytail: derive from the 96-bit UID at 0x1FFF7A10 if that bites. */
    "NAVHAL-0001", /* iSerialNumber */
};

/* -------------------------------------------------------------------------- *
 * Driver state
 * -------------------------------------------------------------------------- */

static volatile uint8_t usb_configured;
static volatile uint8_t usb_line_state; /* DTR/RTS from SET_CONTROL_LINE_STATE */
static volatile uint8_t usb_suspended;  /* bus idle; the host is not polling */
static volatile uint16_t usb_break_ms;  /* break the host asked us to send */
static volatile uint8_t ep_in_busy;     /* a bulk IN transfer is in flight */
static volatile uint8_t ep_notif_busy;  /* a notification is in flight */
static volatile uint8_t out_ep_armed;

static uint8_t rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head, rx_tail;
static hal_usb_cdc_rx_callback_t rx_cb;

/* Line coding: 115200 8N1, little-endian dwDTERate first (CDC PSTN §6.3.11). */
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08};

static uint32_t setup_pkt[2];    /* SETUP packet, word aligned for FIFO reads */
static uint8_t ep0_buf[EP0_MAX_PACKET]; /* OUT data stage / string descriptor */
static uint8_t ctrl_out_request;        /* class request awaiting its OUT data */

/* -------------------------------------------------------------------------- *
 * FIFO access
 * -------------------------------------------------------------------------- */

static void fifo_write(uint8_t fifo, const uint8_t *src, uint16_t len) {
  for (uint16_t i = 0; i < len; i += 4) {
    uint32_t word = 0;
    for (uint16_t b = 0; b < 4 && (i + b) < len; b++)
      word |= (uint32_t)src[i + b] << (8 * b);
    USB_FIFO(fifo) = word;
  }
}

/* Pop @p len bytes from the shared RX FIFO. Always pops whole words — leaving a
 * partial word behind desynchronizes every later packet. */
static void fifo_read(uint8_t *dst, uint16_t len) {
  for (uint16_t i = 0; i < len; i += 4) {
    uint32_t word = USB_FIFO(0);
    for (uint16_t b = 0; b < 4 && (i + b) < len; b++)
      dst[i + b] = (uint8_t)(word >> (8 * b));
  }
}

/* -------------------------------------------------------------------------- *
 * RX ring buffer
 * -------------------------------------------------------------------------- */

static uint16_t ring_used(void) {
  return (uint16_t)((rx_head - rx_tail) & (RX_RING_SIZE - 1U));
}

static uint16_t ring_free(void) { return (RX_RING_SIZE - 1U) - ring_used(); }

static void ring_push(const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    uint16_t next = (uint16_t)((rx_head + 1U) & (RX_RING_SIZE - 1U));
    if (next == rx_tail)
      return; /* full — the OUT endpoint is left un-armed, so this is rare */
    rx_ring[rx_head] = data[i];
    rx_head = next;
  }
}

/* -------------------------------------------------------------------------- *
 * Endpoint helpers
 * -------------------------------------------------------------------------- */

/* Arm EP0 OUT. One shape serves every OUT-side control event: a SETUP packet
 * (STUPCNT), a status-stage ZLP, or a short data stage (SET_LINE_CODING is
 * 7 bytes). A control OUT data stage longer than 24 bytes would be truncated —
 * no request this device answers has one. */
static void ep0_arm(void) {
  USB_OUTEP(EP_CTRL)->TSIZ =
      USB_DOEPTSIZ_STUPCNT(3) | USB_EPTSIZ_PKTCNT(1) | USB_EPTSIZ_XFRSIZ(24);
  USB_OUTEP(EP_CTRL)->CTL |= USB_EPCTL_EPENA | USB_EPCTL_CNAK;
}

/* Queue a control IN data (or status) stage. The whole payload is pushed in one
 * go: the largest descriptor this device answers with is 67 bytes and the EP0
 * TX FIFO holds 256. */
static void ep0_send(const uint8_t *data, uint16_t len, uint16_t requested) {
  if (len > requested)
    len = requested;

  uint32_t packets = len ? ((len + EP0_MAX_PACKET - 1U) / EP0_MAX_PACKET) : 1U;
  /* A short transfer that happens to end on a packet boundary needs a trailing
   * ZLP, or the host keeps waiting for the rest. */
  if (len && len < requested && (len % EP0_MAX_PACKET) == 0U)
    packets++;

  USB_INEP(EP_CTRL)->TSIZ = USB_EPTSIZ_PKTCNT(packets) | USB_EPTSIZ_XFRSIZ(len);
  USB_INEP(EP_CTRL)->CTL |= USB_EPCTL_EPENA | USB_EPCTL_CNAK;
  if (len)
    fifo_write(EP_CTRL, data, len);
}

static void ep0_stall(void) {
  USB_INEP(EP_CTRL)->CTL |= USB_EPCTL_STALL;
  USB_OUTEP(EP_CTRL)->CTL |= USB_EPCTL_STALL;
  ep0_arm();
}

static void out_arm(void) {
  USB_OUTEP(EP_DATA)->TSIZ =
      USB_EPTSIZ_PKTCNT(1) | USB_EPTSIZ_XFRSIZ(HAL_USB_CDC_PACKET_SIZE);
  USB_OUTEP(EP_DATA)->CTL |= USB_EPCTL_EPENA | USB_EPCTL_CNAK;
  out_ep_armed = 1;
}

/* Open the CDC endpoints. Called on SET_CONFIGURATION. */
static void ep_open(void) {
  USB_DEVICE->DAINTMSK |=
      (1U << EP_DATA) | (1U << EP_NOTIF) | (1U << (16U + EP_DATA));

  USB_INEP(EP_DATA)->CTL = USB_EPCTL_MPSIZ(HAL_USB_CDC_PACKET_SIZE) |
                           USB_EPCTL_EPTYP(USB_EPTYP_BULK) |
                           USB_EPCTL_TXFNUM(EP_DATA) | USB_EPCTL_USBAEP |
                           USB_EPCTL_SD0PID | USB_EPCTL_SNAK;
  USB_INEP(EP_NOTIF)->CTL = USB_EPCTL_MPSIZ(NOTIF_MAX_PACKET) |
                            USB_EPCTL_EPTYP(USB_EPTYP_INTERRUPT) |
                            USB_EPCTL_TXFNUM(EP_NOTIF) | USB_EPCTL_USBAEP |
                            USB_EPCTL_SD0PID | USB_EPCTL_SNAK;
  USB_OUTEP(EP_DATA)->CTL = USB_EPCTL_MPSIZ(HAL_USB_CDC_PACKET_SIZE) |
                            USB_EPCTL_EPTYP(USB_EPTYP_BULK) |
                            USB_EPCTL_USBAEP | USB_EPCTL_SD0PID |
                            USB_EPCTL_SNAK;
  ep_in_busy = 0;
  out_arm();
}

static void ep_close(void) {
  USB_DEVICE->DAINTMSK &=
      ~((1U << EP_DATA) | (1U << EP_NOTIF) | (1U << (16U + EP_DATA)));
  USB_INEP(EP_DATA)->CTL = 0;
  USB_INEP(EP_NOTIF)->CTL = 0;
  USB_OUTEP(EP_DATA)->CTL = 0;
  out_ep_armed = 0;
}

/* Halt / un-halt an endpoint by USB address (0x81, 0x01, 0x82).
 *
 * USB 2.0 §9.4.5: clearing a halt also resets the endpoint's data toggle to
 * DATA0. Skipping that leaves the host and device toggles one apart, and every
 * later packet is silently discarded as a retransmission — a stall that looks
 * like a dead link rather than an error. */
static void ep_set_halt(uint8_t addr, uint8_t halt) {
  uint8_t num = addr & 0x0FU;
  if (num == 0U || num > EP_NOTIF)
    return;

  volatile uint32_t *ctl =
      (addr & 0x80U) ? &USB_INEP(num)->CTL : &USB_OUTEP(num)->CTL;

  if (halt) {
    /* An IN endpoint with a transfer in flight has to be torn down first, or
     * the queued packet would still go out. An OUT endpoint must NOT be
     * disabled this way — the core requires the global OUT-NAK handshake for
     * that, and a bare EPDIS leaves EPDIS and EPENA both stuck set, wedging the
     * endpoint for good. Stalling alone is all the standard asks for. */
    if ((addr & 0x80U) && (*ctl & USB_EPCTL_EPENA))
      *ctl |= USB_EPCTL_EPDIS;
    *ctl |= USB_EPCTL_STALL;
  } else {
    *ctl &= ~USB_EPCTL_STALL;
    *ctl |= USB_EPCTL_SD0PID;
  }

  if (addr & 0x80U) {
    if (num == EP_DATA)
      ep_in_busy = 0; /* whatever was queued will never complete now */
    else
      ep_notif_busy = 0;
  } else if (!halt) {
    out_arm();
  } else {
    out_ep_armed = 0;
  }
}

static uint8_t ep_is_halted(uint8_t addr) {
  uint8_t num = addr & 0x0FU;
  if (num == 0U || num > EP_NOTIF)
    return 0;
  uint32_t ctl =
      (addr & 0x80U) ? USB_INEP(num)->CTL : USB_OUTEP(num)->CTL;
  return (ctl & USB_EPCTL_STALL) ? 1U : 0U;
}

/* SET_INTERFACE re-selects an alternate setting, and the host resets its own
 * data toggles when it does. This device has one setting per interface, so the
 * only thing to honour is the toggle reset. */
static void ep_reset_toggles(void) {
  USB_INEP(EP_DATA)->CTL |= USB_EPCTL_SD0PID;
  USB_INEP(EP_NOTIF)->CTL |= USB_EPCTL_SD0PID;
  USB_OUTEP(EP_DATA)->CTL |= USB_EPCTL_SD0PID;
  ep_in_busy = 0;
  ep_notif_busy = 0;
  out_arm();
}

/* -------------------------------------------------------------------------- *
 * Control transfers
 * -------------------------------------------------------------------------- */

/* ASCII -> USB string descriptor (UTF-16LE), built into ep0_buf. */
static uint16_t make_string_desc(const char *s) {
  uint8_t len = 2;
  ep0_buf[1] = 0x03;
  while (*s && len < (EP0_MAX_PACKET - 2)) {
    ep0_buf[len++] = (uint8_t)*s++;
    ep0_buf[len++] = 0;
  }
  ep0_buf[0] = len;
  return len;
}

static void handle_get_descriptor(uint16_t wValue, uint16_t wLength) {
  uint8_t type = (uint8_t)(wValue >> 8);
  uint8_t index = (uint8_t)(wValue & 0xFF);

  switch (type) {
  case 0x01: /* DEVICE */
    ep0_send(device_desc, sizeof(device_desc), wLength);
    break;
  case 0x02: /* CONFIGURATION */
    ep0_send(config_desc, sizeof(config_desc), wLength);
    break;
  case 0x03: /* STRING */
    if (index == 0)
      ep0_send(string_langid, sizeof(string_langid), wLength);
    else if (index <= (sizeof(string_table) / sizeof(string_table[0])))
      ep0_send(ep0_buf, make_string_desc(string_table[index - 1]), wLength);
    else
      ep0_stall();
    break;
  default:
    /* DEVICE_QUALIFIER / OTHER_SPEED_CONFIG: a full-speed-only device must
     * stall these, which is how the host learns it has no high-speed mode. */
    ep0_stall();
    break;
  }
}

static void handle_setup(void) {
  const uint8_t *s = (const uint8_t *)setup_pkt;
  uint8_t type = (uint8_t)((s[0] >> 5) & 0x03);
  uint8_t recipient = (uint8_t)(s[0] & 0x1F);
  uint8_t request = s[1];
  uint16_t wValue = (uint16_t)(s[2] | ((uint16_t)s[3] << 8));
  uint16_t wIndex = (uint16_t)(s[4] | ((uint16_t)s[5] << 8));
  uint16_t wLength = (uint16_t)(s[6] | ((uint16_t)s[7] << 8));

  if (type == 0x01) { /* class request — CDC */
    switch (request) {
    case CDC_SET_LINE_CODING:
      /* The 7 data bytes follow in an OUT packet; ep0_arm() below catches it. */
      ctrl_out_request = request;
      break;
    case CDC_GET_LINE_CODING:
      ep0_send(line_coding, sizeof(line_coding), wLength);
      break;
    case CDC_SET_CONTROL_LINE_STATE:
      usb_line_state = (uint8_t)(wValue & 0x03); /* bit0 DTR, bit1 RTS */
      ep0_send(0, 0, 0);
      break;
    case CDC_SEND_BREAK:
      /* wValue is the duration in ms; 0 revokes it, 0xFFFF means "until I say
       * otherwise". Nothing to drive on a virtual port — a UART bridge reads it
       * back with hal_usb_cdc_get_break_ms(). */
      usb_break_ms = wValue;
      ep0_send(0, 0, 0);
      break;
    default:
      ep0_stall();
      return;
    }
    ep0_arm();
    return;
  }

  if (type != 0x00) { /* vendor requests: none */
    ep0_stall();
    return;
  }

  switch (request) {
  case REQ_GET_DESCRIPTOR:
    handle_get_descriptor(wValue, wLength);
    break;
  case REQ_SET_ADDRESS:
    /* The OTG core wants the address programmed before the status stage. */
    USB_DEVICE->DCFG =
        (USB_DEVICE->DCFG & ~USB_DCFG_DAD_MASK) | USB_DCFG_DAD(wValue & 0x7F);
    ep0_send(0, 0, 0);
    break;
  case REQ_SET_CONFIGURATION:
    if (wValue) {
      ep_open();
      usb_configured = 1;
    } else {
      ep_close();
      usb_configured = 0;
    }
    ep0_send(0, 0, 0);
    break;
  case REQ_GET_CONFIGURATION:
    ep0_buf[0] = usb_configured;
    ep0_send(ep0_buf, 1, wLength);
    break;
  case REQ_GET_STATUS:
    /* Device: bus powered, no remote wakeup. Endpoint: the halt bit, which is
     * how a host confirms a stall it cleared is really gone. */
    ep0_buf[0] = (recipient == 0x02) ? ep_is_halted((uint8_t)wIndex) : 0;
    ep0_buf[1] = 0;
    ep0_send(ep0_buf, 2, wLength);
    break;
  case REQ_GET_INTERFACE:
    ep0_buf[0] = 0;
    ep0_send(ep0_buf, 1, wLength);
    break;
  case REQ_CLEAR_FEATURE:
  case REQ_SET_FEATURE:
    /* ENDPOINT_HALT (feature selector 0) on an endpoint is the only feature
     * this device implements; the host uses it to recover a wedged endpoint.
     * Everything else is accepted and ignored. */
    if (recipient == 0x02 && wValue == 0x00)
      ep_set_halt((uint8_t)wIndex, request == REQ_SET_FEATURE);
    ep0_send(0, 0, 0);
    break;
  case REQ_SET_INTERFACE:
    ep_reset_toggles();
    ep0_send(0, 0, 0);
    break;
  default:
    ep0_stall();
    return;
  }
  ep0_arm();
}

/* -------------------------------------------------------------------------- *
 * Interrupt handling
 * -------------------------------------------------------------------------- */

static void on_reset(void) {
  USB_DEVICE->DCTL &= ~USB_DCTL_RWUSIG;

  for (uint32_t i = 0; i < 4; i++) {
    USB_INEP(i)->CTL = 0;
    USB_INEP(i)->INT = 0xFB7FU;
    USB_OUTEP(i)->CTL = 0;
    USB_OUTEP(i)->INT = 0xFB7FU;
  }

  USB_DEVICE->DAINTMSK = (1U << EP_CTRL) | (1U << (16U + EP_CTRL));
  USB_DEVICE->DOEPMSK =
      USB_EPINT_XFRC | USB_EPINT_EPDISD | USB_DOEPINT_STUP;
  USB_DEVICE->DIEPMSK = USB_EPINT_XFRC | USB_EPINT_EPDISD | USB_DIEPINT_TOC;

  USB_DEVICE->DCFG &= ~USB_DCFG_DAD_MASK; /* back to the default address */

  usb_configured = 0;
  usb_line_state = 0;
  usb_suspended = 0;
  usb_break_ms = 0;
  ep_in_busy = 0;
  ep_notif_busy = 0;
  out_ep_armed = 0;
  ctrl_out_request = 0;
  rx_head = rx_tail = 0;

  ep0_arm();
}

static void on_enum_done(void) {
  /* EP0 MPSIZ is an encoded field: 0 means 64 bytes, the only full-speed
   * control packet size this device uses. */
  USB_INEP(EP_CTRL)->CTL &= ~0x7FFU;
  USB_DEVICE->DCTL |= USB_DCTL_CGINAK;
}

static void on_rxflvl(void) {
  uint8_t packet[HAL_USB_CDC_PACKET_SIZE];

  /* RXFLVL is a level flag over the FIFO, not a latched bit: mask it while
   * draining so the handler cannot re-enter on a packet it is mid-way through. */
  USB_GLOBAL->GINTMSK &= ~USB_GINT_RXFLVL;

  while (USB_GLOBAL->GINTSTS & USB_GINT_RXFLVL) {
    uint32_t sts = USB_GLOBAL->GRXSTSP;
    uint8_t ep = (uint8_t)USB_GRXSTS_EPNUM(sts);
    uint16_t bcnt = (uint16_t)USB_GRXSTS_BCNT(sts);
    uint8_t pktsts = (uint8_t)USB_GRXSTS_PKTSTS(sts);

    if (bcnt > sizeof(packet))
      bcnt = sizeof(packet); /* cannot happen: no endpoint is larger */

    if (pktsts == USB_PKTSTS_SETUP_DATA && bcnt == 8) {
      fifo_read((uint8_t *)setup_pkt, 8);
    } else if (pktsts == USB_PKTSTS_OUT_DATA && bcnt) {
      fifo_read(packet, bcnt);
      if (ep == EP_CTRL) {
        for (uint16_t i = 0; i < bcnt && i < sizeof(ep0_buf); i++)
          ep0_buf[i] = packet[i];
      } else if (ep == EP_DATA) {
        if (rx_cb)
          rx_cb(packet, bcnt);
        else
          ring_push(packet, bcnt);
      }
    } else if (bcnt) {
      fifo_read(packet, bcnt); /* unexpected payload: drain, never leave it */
    }
  }

  USB_GLOBAL->GINTMSK |= USB_GINT_RXFLVL;
}

static void on_out_ep_int(void) {
  uint32_t pending = (USB_DEVICE->DAINT & USB_DEVICE->DAINTMSK) >> 16;

  for (uint32_t ep = 0; ep < 4U; ep++) {
    if (!(pending & (1U << ep)))
      continue;
    uint32_t flags = USB_OUTEP(ep)->INT;

    if (flags & USB_DOEPINT_STUP) {
      USB_OUTEP(ep)->INT = USB_DOEPINT_STUP | USB_EPINT_XFRC;
      handle_setup();
      continue;
    }
    if (flags & USB_EPINT_XFRC) {
      USB_OUTEP(ep)->INT = USB_EPINT_XFRC;
      if (ep == EP_CTRL) {
        if (ctrl_out_request == CDC_SET_LINE_CODING) {
          for (uint8_t i = 0; i < sizeof(line_coding); i++)
            line_coding[i] = ep0_buf[i];
          ctrl_out_request = 0;
          ep0_send(0, 0, 0); /* status stage */
        }
        ep0_arm();
      } else if (ep == EP_DATA) {
        /* Re-arm only while the ring can take another full packet; otherwise
         * leave the endpoint NAKing and let hal_usb_cdc_read() re-arm it. That
         * is the flow control that keeps the host from overrunning us. */
        if (ring_free() >= HAL_USB_CDC_PACKET_SIZE)
          out_arm();
        else
          out_ep_armed = 0;
      }
    }
    USB_OUTEP(ep)->INT = flags; /* clear anything else that latched */
  }
}

static void on_in_ep_int(void) {
  uint32_t pending = USB_DEVICE->DAINT & USB_DEVICE->DAINTMSK & 0xFFFFU;

  for (uint32_t ep = 0; ep < 4U; ep++) {
    if (!(pending & (1U << ep)))
      continue;
    uint32_t flags = USB_INEP(ep)->INT;

    if (flags & USB_EPINT_XFRC) {
      USB_INEP(ep)->INT = USB_EPINT_XFRC;
      if (ep == EP_DATA)
        ep_in_busy = 0;
      else if (ep == EP_NOTIF)
        ep_notif_busy = 0;
    }
    USB_INEP(ep)->INT = flags & ~USB_DIEPINT_TXFE;
  }
}

static void usb_irq_handler(void) {
  uint32_t sts = USB_GLOBAL->GINTSTS & USB_GLOBAL->GINTMSK;

  if (sts & USB_GINT_USBRST) {
    USB_GLOBAL->GINTSTS = USB_GINT_USBRST;
    on_reset();
  }
  if (sts & USB_GINT_ENUMDNE) {
    USB_GLOBAL->GINTSTS = USB_GINT_ENUMDNE;
    on_enum_done();
  }
  if (sts & USB_GINT_RXFLVL)
    on_rxflvl();
  if (sts & USB_GINT_OEPINT)
    on_out_ep_int();
  if (sts & USB_GINT_IEPINT)
    on_in_ep_int();
  if (sts & USB_GINT_USBSUSP) {
    USB_GLOBAL->GINTSTS = USB_GINT_USBSUSP;
    /* Suspend does not close the port: the host never re-sends DTR on resume,
     * so clearing the line state here would leave the device mute forever.
     * Track it separately and let it lift on the resume interrupt. */
    usb_suspended = 1;
  }
  if (sts & USB_GINT_WKUPINT)
    usb_suspended = 0;
  if (sts & (USB_GINT_WKUPINT | USB_GINT_ESUSP | USB_GINT_SOF | USB_GINT_MMIS |
             USB_GINT_OTGINT))
    USB_GLOBAL->GINTSTS = USB_GINT_WKUPINT | USB_GINT_ESUSP | USB_GINT_SOF |
                          USB_GINT_MMIS | USB_GINT_OTGINT;
}

/* -------------------------------------------------------------------------- *
 * Core bring-up
 * -------------------------------------------------------------------------- */

/* The USB transceiver needs exactly 48 MHz off the PLL Q output; anything else
 * enumerates unreliably or not at all, with no error to report at run time. */
static uint32_t usb_clock_hz(void) {
  if (!(RCC->CR & RCC_CR_PLLRDY))
    return 0;
  uint32_t m = RCC->PLLCFGR & 0x3FU;
  uint32_t n = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLN_BIT) & 0x1FFU;
  uint32_t q = (RCC->PLLCFGR >> RCC_PLLCFGR_PLLQ_BIT) & 0x0FU;
  if (!m || !q)
    return 0;
  uint32_t vco_in = (RCC->PLLCFGR & RCC_PLLCFGR_SRC) ? BOARD_HSE_FREQ_HZ
                                                     : BOARD_HSI_FREQ_HZ;
  return (vco_in / m) * n / q;
}

static hal_status_t core_reset(void) {
  uint32_t spins = CORE_TIMEOUT_SPINS;
  while (!(USB_GLOBAL->GRSTCTL & USB_GRSTCTL_AHBIDL))
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;

  USB_GLOBAL->GRSTCTL |= USB_GRSTCTL_CSRST;
  spins = CORE_TIMEOUT_SPINS;
  while (USB_GLOBAL->GRSTCTL & USB_GRSTCTL_CSRST)
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;
  return HAL_OK;
}

static hal_status_t flush_fifos(void) {
  uint32_t spins = CORE_TIMEOUT_SPINS;
  USB_GLOBAL->GRSTCTL = USB_GRSTCTL_TXFNUM(0x10) | USB_GRSTCTL_TXFFLSH;
  while (USB_GLOBAL->GRSTCTL & USB_GRSTCTL_TXFFLSH)
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;

  spins = CORE_TIMEOUT_SPINS;
  USB_GLOBAL->GRSTCTL = USB_GRSTCTL_RXFFLSH;
  while (USB_GLOBAL->GRSTCTL & USB_GRSTCTL_RXFFLSH)
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;
  return HAL_OK;
}

hal_status_t hal_usb_cdc_init(void) {
  if (usb_clock_hz() != 48000000U)
    return HAL_ERR_NOT_INITIALIZED;

  /* PA11 = DM, PA12 = DP, both AF10 (OTG_FS). */
  const hal_gpio_pin_t pins[2] = {GPIO_PA11, GPIO_PA12};
  for (uint32_t i = 0; i < 2; i++) {
    hal_gpio_set_output_speed(pins[i], HAL_GPIO_SPEED_VERY_HIGH);
    hal_gpio_set_output_type(pins[i], HAL_GPIO_OTYPE_PUSH_PULL);
    hal_gpio_set_alternate_function(pins[i], HAL_GPIO_AF10);
    hal_gpio_set_mode(pins[i], HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
  }

  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

  USB_GLOBAL->GAHBCFG &= ~USB_GAHBCFG_GINT; /* configure with interrupts off */

  /* Power up the transceiver and run without VBUS sensing: the core then holds
   * the session valid itself, which is what a bus-powered device wants. */
  USB_GLOBAL->GCCFG = USB_GCCFG_PWRDWN | USB_GCCFG_NOVBUSSENS;

  /* PHYSEL selects the embedded full-speed transceiver and must be set before
   * the core reset. */
  USB_GLOBAL->GUSBCFG |= USB_GUSBCFG_PHYSEL;
  HAL_OK_OR_RETURN(core_reset());

  /* Device mode, turnaround time for a 48 MHz full-speed PHY (RM0368 §22.16.2).
   * The core needs time to settle after a mode change before the device
   * registers are touched. */
  USB_GLOBAL->GUSBCFG =
      USB_GUSBCFG_PHYSEL | USB_GUSBCFG_FDMOD | USB_GUSBCFG_TRDT(6);
  for (volatile uint32_t i = 0; i < CORE_TIMEOUT_SPINS; i++)
    ;

  USB_PCGCCTL = 0; /* ungate the PHY and core clocks */
  USB_DEVICE->DCFG = USB_DCFG_DSPD_FS | USB_DCFG_NZLSOHSK;

  USB_GLOBAL->GRXFSIZ = FIFO_RX_WORDS;
  USB_GLOBAL->DIEPTXF0 = USB_FIFOSIZE(FIFO_RX_WORDS, FIFO_TX0_WORDS);
  USB_GLOBAL->DIEPTXF[0] =
      USB_FIFOSIZE(FIFO_RX_WORDS + FIFO_TX0_WORDS, FIFO_TX1_WORDS);
  USB_GLOBAL->DIEPTXF[1] = USB_FIFOSIZE(
      FIFO_RX_WORDS + FIFO_TX0_WORDS + FIFO_TX1_WORDS, FIFO_TX2_WORDS);

  HAL_OK_OR_RETURN(flush_fifos());

  USB_DEVICE->DIEPMSK = 0;
  USB_DEVICE->DOEPMSK = 0;
  USB_DEVICE->DAINTMSK = 0;
  for (uint32_t i = 0; i < 4; i++) {
    USB_INEP(i)->CTL = 0;
    USB_INEP(i)->TSIZ = 0;
    USB_INEP(i)->INT = 0xFB7FU;
    USB_OUTEP(i)->CTL = 0;
    USB_OUTEP(i)->TSIZ = 0;
    USB_OUTEP(i)->INT = 0xFB7FU;
  }

  USB_GLOBAL->GINTSTS = 0xFFFFFFFFU;
  USB_GLOBAL->GINTMSK = USB_GINT_USBRST | USB_GINT_ENUMDNE | USB_GINT_RXFLVL |
                        USB_GINT_IEPINT | USB_GINT_OEPINT | USB_GINT_USBSUSP |
                        USB_GINT_WKUPINT;

  hal_interrupt_attach_callback(OTG_FS_IRQn, usb_irq_handler);
  hal_interrupt_enable(OTG_FS_IRQn);

  USB_GLOBAL->GAHBCFG |= USB_GAHBCFG_GINT;
  USB_DEVICE->DCTL &= ~USB_DCTL_SDIS; /* release D+ — the host sees us now */

  return HAL_OK;
}

hal_status_t hal_usb_cdc_deinit(void) {
  USB_DEVICE->DCTL |= USB_DCTL_SDIS;
  USB_GLOBAL->GAHBCFG &= ~USB_GAHBCFG_GINT;
  hal_interrupt_disable(OTG_FS_IRQn);
  hal_interrupt_detach_callback(OTG_FS_IRQn);
  USB_GLOBAL->GCCFG = 0;
  RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;
  usb_configured = 0;
  usb_line_state = 0;
  return HAL_OK;
}

/* -------------------------------------------------------------------------- *
 * Public data path
 * -------------------------------------------------------------------------- */

bool hal_usb_cdc_connected(void) {
  return usb_configured && !usb_suspended &&
         (usb_line_state & HAL_USB_CDC_LINE_DTR);
}

static hal_status_t ep_in_transfer(const uint8_t *data, uint16_t len) {
  uint32_t spins = TX_TIMEOUT_SPINS;
  while (ep_in_busy) {
    if (!usb_configured)
      return HAL_ERR_NOT_INITIALIZED;
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;
  }

  /* Arm + fill is one sequence, and the handler runs the same sequence for EP0
   * on any control transfer that lands mid-write. Keeping it atomic costs ~64
   * word writes with interrupts masked. */
  uint32_t state = hal_interrupt_disable_global();
  ep_in_busy = 1;
  USB_INEP(EP_DATA)->TSIZ =
      USB_EPTSIZ_PKTCNT(len ? ((len + HAL_USB_CDC_PACKET_SIZE - 1U) /
                               HAL_USB_CDC_PACKET_SIZE)
                            : 1U) |
      USB_EPTSIZ_XFRSIZ(len);
  USB_INEP(EP_DATA)->CTL |= USB_EPCTL_EPENA | USB_EPCTL_CNAK;
  if (len)
    fifo_write(EP_DATA, data, len);
  hal_interrupt_enable_global(state);
  return HAL_OK;
}

hal_status_t hal_usb_cdc_write(const uint8_t *data, uint16_t length) {
  if (!data && length)
    return HAL_ERR_INVALID_ARG;
  if (!usb_configured)
    return HAL_ERR_NOT_INITIALIZED;

  uint16_t sent = 0;
  while (sent < length) {
    uint16_t chunk = (uint16_t)(length - sent);
    if (chunk > TX_CHUNK)
      chunk = TX_CHUNK;
    HAL_OK_OR_RETURN(ep_in_transfer(data + sent, chunk));
    sent = (uint16_t)(sent + chunk);
  }
  /* A write that ends exactly on a packet boundary needs a zero-length packet
   * so the host's read returns instead of waiting for the rest. */
  if (length && (length % HAL_USB_CDC_PACKET_SIZE) == 0U)
    HAL_OK_OR_RETURN(ep_in_transfer(0, 0));

  return HAL_OK;
}

hal_status_t hal_usb_cdc_write_string(const char *s) {
  if (!s)
    return HAL_ERR_INVALID_ARG;
  uint16_t len = 0;
  while (s[len])
    len++;
  return hal_usb_cdc_write((const uint8_t *)s, len);
}

uint16_t hal_usb_cdc_read(uint8_t *buffer, uint16_t maxlen) {
  if (!buffer)
    return 0;

  uint16_t n = 0;
  while (n < maxlen && rx_tail != rx_head) {
    buffer[n++] = rx_ring[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1U) & (RX_RING_SIZE - 1U));
  }

  /* Draining the ring is what re-opens the OUT endpoint after a stall-out. */
  if (!out_ep_armed && usb_configured &&
      ring_free() >= HAL_USB_CDC_PACKET_SIZE) {
    uint32_t state = hal_interrupt_disable_global();
    if (!out_ep_armed)
      out_arm();
    hal_interrupt_enable_global(state);
  }
  return n;
}

uint16_t hal_usb_cdc_available(void) { return ring_used(); }

hal_status_t hal_usb_cdc_set_rx_callback(hal_usb_cdc_rx_callback_t cb) {
  rx_cb = cb;
  return HAL_OK;
}

hal_status_t hal_usb_cdc_get_line_coding(hal_usb_cdc_line_coding_t *out) {
  if (!out)
    return HAL_ERR_INVALID_ARG;
  out->baudrate = hal_usb_cdc_get_baudrate();
  out->stop_bits = line_coding[4];
  out->parity = line_coding[5];
  out->data_bits = line_coding[6];
  return HAL_OK;
}

uint8_t hal_usb_cdc_get_line_state(void) { return usb_line_state; }

uint16_t hal_usb_cdc_get_break_ms(void) { return usb_break_ms; }

hal_status_t hal_usb_cdc_notify_serial_state(uint16_t state) {
  if (!usb_configured)
    return HAL_ERR_NOT_INITIALIZED;

  /* Class notification header (CDC 1.2 §6.3): interface request, SERIAL_STATE,
   * addressed to the communication interface, carrying a 2-byte bitmap. */
  const uint8_t packet[10] = {
      0xA1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
      (uint8_t)(state & 0xFF), (uint8_t)(state >> 8),
  };

  uint32_t spins = TX_TIMEOUT_SPINS;
  while (ep_notif_busy) {
    if (!usb_configured)
      return HAL_ERR_NOT_INITIALIZED;
    if (--spins == 0)
      return HAL_ERR_TIMEOUT;
  }

  uint32_t irq = hal_interrupt_disable_global();
  ep_notif_busy = 1;
  USB_INEP(EP_NOTIF)->TSIZ =
      USB_EPTSIZ_PKTCNT((sizeof(packet) + NOTIF_MAX_PACKET - 1U) /
                        NOTIF_MAX_PACKET) |
      USB_EPTSIZ_XFRSIZ(sizeof(packet));
  USB_INEP(EP_NOTIF)->CTL |= USB_EPCTL_EPENA | USB_EPCTL_CNAK;
  fifo_write(EP_NOTIF, packet, sizeof(packet));
  hal_interrupt_enable_global(irq);
  return HAL_OK;
}

uint32_t hal_usb_cdc_get_baudrate(void) {
  return (uint32_t)line_coding[0] | ((uint32_t)line_coding[1] << 8) |
         ((uint32_t)line_coding[2] << 16) | ((uint32_t)line_coding[3] << 24);
}

#endif /* NAVHAL_CONFIG_DRV_USB_CDC */
