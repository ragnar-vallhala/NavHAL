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
 * @file hal_usb_cdc.h
 * @brief Portable HAL interface for a USB CDC-ACM virtual serial port.
 *
 * @details
 * The target enumerates as a USB full-speed CDC-ACM device — a virtual COM
 * port (`/dev/ttyACM0` on Linux, a COM port on Windows). There is one such
 * device per target, so — like ::hal_sdio — the functions take no instance id.
 *
 * Receive is interrupt-driven into an internal ring buffer: ::hal_usb_cdc_read
 * never blocks and returns what has arrived so far. Transmit copies into the
 * peripheral's TX FIFO and waits for the host to collect the previous packet,
 * with a bounded spin so an un-drained port cannot hang the caller forever.
 *
 * The host controls the line: nothing is sent until the port has been opened
 * (::hal_usb_cdc_connected). The reported baud rate, parity and stop bits of a
 * CDC port are cosmetic — there is no real UART behind them — so the requested
 * line coding is accepted and ignored.
 *
 * The whole API compiles only when @c NAVHAL_CONFIG_DRV_USB_CDC is set.
 *
 * ### Typical usage
 * @code
 * hal_usb_cdc_init();
 * while (!hal_usb_cdc_connected()) { }
 * hal_usb_cdc_write_string("hello over USB\r\n");
 *
 * uint8_t buf[64];
 * uint16_t n = hal_usb_cdc_read(buf, sizeof(buf));
 * if (n)
 *   hal_usb_cdc_write(buf, n);   // echo
 * @endcode
 */

#ifndef HAL_USB_CDC_H
#define HAL_USB_CDC_H

/**
 * @defgroup HAL_USB_CDC UsbCdc
 * @ingroup HAL_DRIVERS
 * @brief USB CDC-ACM virtual serial port (device side).
 * @{
 */

#include "common/hal_config.h"
#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_USB_CDC

/** @brief Bulk endpoint packet size (USB full speed). */
#define HAL_USB_CDC_PACKET_SIZE 64

/**
 * @brief Called from interrupt context when bytes arrive.
 *
 * @param data Received bytes (valid only for the duration of the call).
 * @param len  Number of bytes.
 *
 * Bytes handed to the callback are *not* also queued for ::hal_usb_cdc_read.
 */
typedef void (*hal_usb_cdc_rx_callback_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Bring up the USB device core and attach to the bus.
 *
 * Configures the OTG_FS pins (PA11/PA12), resets and initializes the core in
 * device mode, and releases the D+ pull-up. Enumeration then proceeds in the
 * background from the OTG_FS interrupt; poll ::hal_usb_cdc_connected for it.
 *
 * @return ::HAL_OK, or ::HAL_ERR_NOT_INITIALIZED when the 48 MHz USB clock is
 *         not running (RCC PLL Q output must be exactly 48 MHz), or
 *         ::HAL_ERR_TIMEOUT if the core never leaves reset.
 */
hal_status_t hal_usb_cdc_init(void);

/**
 * @brief Detach from the bus and power the transceiver down.
 * @return ::HAL_OK.
 */
hal_status_t hal_usb_cdc_deinit(void);

/**
 * @brief Whether the host has enumerated the device and opened the port.
 * @return true once the device is configured and the host has asserted DTR.
 */
bool hal_usb_cdc_connected(void);

/**
 * @brief Send a byte buffer to the host.
 *
 * Blocks until the data has been handed to the USB core, up to an internal
 * timeout. A transfer whose length is a multiple of ::HAL_USB_CDC_PACKET_SIZE
 * is terminated with a zero-length packet, so the host sees the write boundary.
 *
 * @param data   Source buffer.
 * @param length Number of bytes.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG on a NULL buffer,
 *         ::HAL_ERR_NOT_INITIALIZED if the port is not open, or
 *         ::HAL_ERR_TIMEOUT if the host stopped collecting data.
 */
hal_status_t hal_usb_cdc_write(const uint8_t *data, uint16_t length);

/** @brief Send a null-terminated string. */
hal_status_t hal_usb_cdc_write_string(const char *s);

/**
 * @brief Take received bytes from the RX ring buffer (never blocks).
 * @param buffer Destination.
 * @param maxlen Capacity of @p buffer.
 * @return Number of bytes copied; 0 when nothing has arrived.
 */
uint16_t hal_usb_cdc_read(uint8_t *buffer, uint16_t maxlen);

/** @brief Number of bytes waiting in the RX ring buffer. */
uint16_t hal_usb_cdc_available(void);

/**
 * @brief Deliver received bytes to @p cb from interrupt context instead of
 *        queueing them for ::hal_usb_cdc_read.
 * @param cb Callback, or NULL to go back to the ring buffer.
 * @return ::HAL_OK.
 */
hal_status_t hal_usb_cdc_set_rx_callback(hal_usb_cdc_rx_callback_t cb);

/**
 * @brief Baud rate the host last asked for (cosmetic — see the file notes).
 * @return Baud rate in bits per second; 115200 until the host sets one.
 */
uint32_t hal_usb_cdc_get_baudrate(void);

#endif /* NAVHAL_CONFIG_DRV_USB_CDC */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_USB_CDC */
#endif /* HAL_USB_CDC_H */
