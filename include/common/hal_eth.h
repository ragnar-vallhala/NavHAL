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
 * @file hal_eth.h
 * @brief Portable HAL interface for the Ethernet MAC.
 *
 * @details
 * Standardized Ethernet API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_eth_ prefix and return ::hal_status_t. The driver
 * operates at the frame level: the caller sends and receives complete Ethernet
 * frames (destination MAC, source MAC, ethertype/length, payload), and a
 * network stack layers on top. There is a single MAC per target, so — like
 * @ref HAL_SDIO — the functions take no instance id.
 *
 * The MAC drives its own dedicated DMA with descriptor rings, separate from the
 * general-purpose controller in @c hal_dma. Frame transfers are therefore
 * always descriptor-based: ::hal_eth_send hands a frame to a TX descriptor and
 * ::hal_eth_receive takes the next completed RX descriptor. Neither blocks on
 * the wire.
 *
 * The link is managed through the PHY over MDIO: ::hal_eth_get_link reports the
 * negotiated speed/duplex, and ::hal_eth_phy_read / ::hal_eth_phy_write reach
 * the PHY's registers directly.
 *
 * The entire API is compiled only when @c NAVHAL_CONFIG_DRV_ETH is set; on a
 * target without an Ethernet MAC the header collapses to nothing.
 *
 * @note The descriptor rings and frame buffers live in memory the MAC DMA
 *       accesses directly. With the L1 D-cache enabled they need clean/
 *       invalidate maintenance or placement in a non-cached region (e.g. DTCM);
 *       the bring-up default keeps the D-cache off, so they are coherent.
 *
 * ### Typical usage
 * @code
 * hal_eth_config_t cfg = {
 *     .mac_addr        = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
 *     .interface       = HAL_ETH_IFACE_RMII,
 *     .phy_address     = 0,
 *     .auto_negotiation = true,
 * };
 * hal_eth_init(&cfg);
 * hal_eth_start();
 *
 * uint8_t frame[HAL_ETH_MAX_FRAME_LEN];
 * uint16_t len = 0;
 * if (hal_eth_receive(frame, sizeof(frame), &len) == HAL_OK && len > 0) {
 *     // ... process len bytes ...
 *     hal_eth_send(frame, len);
 * }
 * @endcode
 */

#ifndef HAL_ETH_H
#define HAL_ETH_H

/**
 * @defgroup HAL_ETH Eth
 * @ingroup HAL_DRIVERS
 * @brief Ethernet MAC (frame-level media access).
 * @{
 */

#include "common/hal_config.h" /* sources the NAVHAL_CONFIG_DRV_ETH capability flag */
#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_ETH

/** @brief Length of a 48-bit MAC address in bytes. */
#define HAL_ETH_MAC_ADDR_LEN 6

/** @brief Largest frame the MAC accepts/delivers: 1500 MTU + 14 header + 4 FCS
 *         + 4 optional VLAN tag. */
#define HAL_ETH_MAX_FRAME_LEN 1522

/** @brief Smallest valid Ethernet frame (excluding the 4-byte FCS). */
#define HAL_ETH_MIN_FRAME_LEN 60

/** @brief Link speed. The STM32 MAC is 10/100 (no gigabit). */
typedef enum {
  HAL_ETH_SPEED_10M = 0,  /**< 10 Mbit/s. */
  HAL_ETH_SPEED_100M = 1, /**< 100 Mbit/s. */
} hal_eth_speed_t;

/** @brief Link duplex mode. */
typedef enum {
  HAL_ETH_HALF_DUPLEX = 0, /**< Half duplex. */
  HAL_ETH_FULL_DUPLEX = 1, /**< Full duplex. */
} hal_eth_duplex_t;

/** @brief MAC-to-PHY interface wiring. */
typedef enum {
  HAL_ETH_IFACE_MII = 0,  /**< Media-independent interface. */
  HAL_ETH_IFACE_RMII = 1, /**< Reduced media-independent interface. */
} hal_eth_phy_iface_t;

/** @brief Asynchronous Ethernet events delivered to the registered callback. */
typedef enum {
  HAL_ETH_EVENT_RX = 0,      /**< One or more frames are available to receive. */
  HAL_ETH_EVENT_TX_DONE = 1, /**< A queued frame finished transmitting. */
  HAL_ETH_EVENT_LINK_UP = 2, /**< The PHY link came up. */
  HAL_ETH_EVENT_LINK_DOWN = 3, /**< The PHY link went down. */
  HAL_ETH_EVENT_ERROR = 4,   /**< A DMA / bus error occurred. */
} hal_eth_event_t;

/**
 * @brief Ethernet initialization configuration.
 *
 * Zero-initializable to sane defaults: MII interface, PHY address 0, forced
 * 100M full duplex. Set @c auto_negotiation to let the PHY choose speed/duplex.
 */
typedef struct {
  uint8_t mac_addr[HAL_ETH_MAC_ADDR_LEN]; /**< Station MAC address. */
  hal_eth_phy_iface_t interface;          /**< MII or RMII wiring. */
  uint8_t phy_address;                    /**< PHY address on the MDIO bus. */
  bool auto_negotiation;                  /**< Auto-negotiate speed/duplex. */
  hal_eth_speed_t speed;   /**< Forced speed when @c auto_negotiation is false. */
  hal_eth_duplex_t duplex; /**< Forced duplex when @c auto_negotiation is false. */
  bool promiscuous;        /**< Receive all frames, not just addressed ones. */
} hal_eth_config_t;

/** @brief Current link state (see ::hal_eth_get_link). */
typedef struct {
  bool up;                 /**< True when the PHY link is up. */
  hal_eth_speed_t speed;   /**< Negotiated / configured speed. */
  hal_eth_duplex_t duplex; /**< Negotiated / configured duplex. */
} hal_eth_link_t;

/**
 * @brief Ethernet event callback.
 * @param event The event that occurred.
 */
typedef void (*hal_eth_callback_t)(hal_eth_event_t event);

/**
 * @brief Initialize the MAC, its DMA descriptor rings, the RMII/MII GPIOs, and
 *        the PHY.
 * @param config Configuration; must not be NULL.
 * @return ::HAL_OK on success; ::HAL_ERR_INVALID_ARG if @p config is NULL;
 *         ::HAL_ERR_NOT_INITIALIZED if already initialized; ::HAL_ERR_TIMEOUT if
 *         the PHY does not respond.
 */
hal_status_t hal_eth_init(const hal_eth_config_t *config);

/**
 * @brief Disable the MAC and release its resources.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_eth_deinit(void);

/**
 * @brief Enable MAC transmit/receive and start the DMA engines.
 * @return ::HAL_OK, or ::HAL_ERR_NOT_INITIALIZED if called before init.
 */
hal_status_t hal_eth_start(void);

/**
 * @brief Disable MAC transmit/receive and stop the DMA engines.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_eth_stop(void);

/**
 * @brief Register the callback for asynchronous Ethernet events.
 *
 * Set this before ::hal_eth_start to run in interrupt-driven mode: the callback
 * is invoked from the MAC ISR on RX/TX/link/error events and is expected to
 * drain received frames with ::hal_eth_receive. With no callback registered at
 * start time the driver runs in polling mode (interrupts off) and the caller
 * polls ::hal_eth_receive itself.
 *
 * @param callback Function invoked on RX/TX/link/error events; NULL to clear.
 * @return ::HAL_OK.
 */
hal_status_t hal_eth_set_callback(hal_eth_callback_t callback);

/**
 * @brief Queue one frame for transmission (non-blocking).
 *
 * The frame is copied into a free TX descriptor buffer and handed to the MAC
 * DMA; the call returns without waiting for the wire.
 *
 * @param frame Complete frame starting at the destination MAC; the MAC appends
 *              the FCS, so it must not be included.
 * @param len   Frame length in bytes (::HAL_ETH_MIN_FRAME_LEN ..
 *              ::HAL_ETH_MAX_FRAME_LEN); short frames are padded by the MAC.
 * @return ::HAL_OK if queued; ::HAL_ERR_INVALID_ARG on a NULL/oversized frame;
 *         ::HAL_ERR_BUSY if no TX descriptor is free;
 *         ::HAL_ERR_NOT_INITIALIZED if the MAC is not started.
 */
hal_status_t hal_eth_send(const uint8_t *frame, uint16_t len);

/**
 * @brief Take the next received frame, if any (non-blocking).
 *
 * @param buf     Destination buffer.
 * @param max_len Capacity of @p buf in bytes.
 * @param out_len Set to the frame length on success, or 0 when no frame is
 *                pending; must not be NULL.
 * @return ::HAL_OK when a frame was copied (@p out_len > 0) or none was pending
 *         (@p out_len == 0); ::HAL_ERR_INVALID_ARG on NULL arguments;
 *         ::HAL_ERR_NO_MEM if the pending frame is larger than @p max_len;
 *         ::HAL_ERR_NOT_INITIALIZED if the MAC is not started.
 */
hal_status_t hal_eth_receive(uint8_t *buf, uint16_t max_len, uint16_t *out_len);

/**
 * @brief Read the current link state.
 * @param out Destination; must not be NULL.
 * @return ::HAL_OK, or an error status.
 */
hal_status_t hal_eth_get_link(hal_eth_link_t *out);

/**
 * @brief Fast link-up check.
 * @return True if the PHY link is up. (Hot-path getter — no error channel.)
 */
bool hal_eth_link_is_up(void);

/**
 * @brief Set the station MAC address.
 * @param mac 6-byte address; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_eth_set_mac_address(const uint8_t mac[HAL_ETH_MAC_ADDR_LEN]);

/**
 * @brief Get the station MAC address.
 * @param mac 6-byte destination; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_eth_get_mac_address(uint8_t mac[HAL_ETH_MAC_ADDR_LEN]);

/**
 * @brief Read a PHY register over MDIO.
 * @param reg PHY register address (0-31).
 * @param out Destination for the 16-bit value; must not be NULL.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG, or ::HAL_ERR_TIMEOUT.
 */
hal_status_t hal_eth_phy_read(uint8_t reg, uint16_t *out);

/**
 * @brief Write a PHY register over MDIO.
 * @param reg PHY register address (0-31).
 * @param val 16-bit value to write.
 * @return ::HAL_OK, or ::HAL_ERR_TIMEOUT.
 */
hal_status_t hal_eth_phy_write(uint8_t reg, uint16_t val);

#endif /* NAVHAL_CONFIG_DRV_ETH */

#ifdef __cplusplus
} /* extern "C" */
#endif


/** @} */ /* end of group HAL_ETH */
#endif /* HAL_ETH_H */
