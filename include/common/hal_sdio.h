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
 * @file hal_sdio.h
 * @brief Portable HAL interface for SDIO.
 *
 * @details
 * Standardized SDIO API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_sdio_ prefix. Supports 1-bit and 4-bit bus widths
 * and (when the port's DMA backend is enabled) asynchronous block transfers.
 *
 * The entire API is compiled only when @c NAVHAL_CONFIG_DRV_SDIO is defined (see
 * @c NAVHAL_CONFIG_DRV_SDIO) — on a target without an SDIO peripheral the header
 * collapses to nothing, exactly as @c hal_dma.h does for DMA.
 *
 * @note SDIO returns the driver-specific ::hal_sdio_error_t rather than
 *       ::hal_status_t — its asynchronous model needs ::HAL_SDIO_PENDING,
 *       which the standard status enum cannot express. Flagged for the M5
 *       conformance review.
 */

#ifndef HAL_SDIO_H
#define HAL_SDIO_H

/**
 * @defgroup HAL_SDIO Sdio
 * @ingroup HAL_DRIVERS
 * @brief Secure Digital I/O controller for SD-card access.
 * @{
 */

#include "common/hal_config.h" /* NAVHAL_CONFIG_DRV_SDIO: force-included, or pulled here when reachable */
#include <stdbool.h>
/* hal_status_t and NAVHAL_DEPRECATED arrived with the attach/detach pair:
 * every callback function here used to return void. */
#include "common/hal_status.h"
#include "common/hal_sdio_types.h"
#include "common/navhal_compiler.h"
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_DRV_SDIO

/* --- SD Commands --- */
#define SD_CMD_GO_IDLE_STATE 0
#define SD_CMD_ALL_SEND_CID 2
#define SD_CMD_SEND_REL_ADDR 3
#define SD_CMD_SELECT_DESELECT_CARD 7
#define SD_CMD_HS_SEND_EXT_CSD 8
#define SD_CMD_STOP_TRANSMISSION 12
#define SD_CMD_SEND_STATUS 13
#define SD_CMD_SET_BLOCKLEN 16
#define SD_CMD_READ_SINGLE_BLOCK 17
#define SD_CMD_READ_MULT_BLOCK 18
#define SD_CMD_WRITE_SINGLE_BLOCK 24
#define SD_CMD_WRITE_MULT_BLOCK 25
#define SD_CMD_APP_CMD 55
#define SD_ACMD_SD_SEND_OP_COND 41
#define SD_ACMD_SET_BUS_WIDTH 6





/**
 * @brief Set the callback for asynchronous SDIO operations.
 * @param callback Function invoked when an async operation completes.
 */
/**
 * @brief Attach the completion callback for asynchronous SDIO operations.
 * @param callback Non-NULL; pass ::hal_sdio_detach_callback to clear.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL callback.
 */
hal_status_t hal_sdio_attach_callback(hal_sdio_callback_t callback);

/** @brief Clear the completion callback. @return ::HAL_OK. */
hal_status_t hal_sdio_detach_callback(void);

/** @deprecated Use ::hal_sdio_attach_callback / ::hal_sdio_detach_callback. */
NAVHAL_DEPRECATED("use hal_sdio_attach_callback")
static inline void hal_sdio_set_callback(hal_sdio_callback_t callback) {
  if (callback == NULL)
    (void)hal_sdio_detach_callback();
  else
    (void)hal_sdio_attach_callback(callback);
}

/**
 * @brief Initialize the SDIO peripheral and its GPIOs.
 * @param config Configuration; must not be NULL.
 * @return Operation status.
 */
hal_sdio_error_t hal_sdio_init(const hal_sdio_config_t *config);

/**
 * @brief Run the full SD-card initialization sequence (CMD0/8/ACMD41/2/3/7).
 * @return Initialization status.
 */
/**
 * @brief Whether a card is in the slot.
 *
 * Reads the board's card-detect pin, named by @c BOARD_SD_CD. The switch closes
 * to ground when a card is inserted, so the line reads low with a card present.
 *
 * @return @c true when a card is detected, and also on a board that defines no
 *         @c BOARD_SD_CD — with nothing to read, "no card" cannot be claimed,
 *         and reporting an absent card that is actually there would be worse
 *         than not knowing.
 */
bool hal_sdio_card_present(void);

hal_sdio_error_t hal_sdio_card_init(void);

/**
 * @brief Send a command to the SD card.
 * @param cmd_index Command index (0-63).
 * @param argument  Command argument.
 * @param wait_resp Response type (none / short / long).
 * @return Command transmission status.
 */
hal_sdio_error_t hal_sdio_send_command(uint8_t cmd_index, uint32_t argument,
                                       uint32_t wait_resp);

/**
 * @brief Get a response register from the last command.
 * @param response_reg Response register index (1-4).
 * @return Response value.
 */
uint32_t hal_sdio_get_response(uint8_t response_reg);

/**
 * @brief Wait for an SDIO status flag.
 * @param flag    Status flag to wait for.
 * @param timeout Maximum wait time.
 * @return ::HAL_SDIO_OK if the flag was set, ::HAL_SDIO_TIMEOUT otherwise.
 */
hal_sdio_error_t hal_sdio_wait_flag(uint32_t flag, uint32_t timeout);

/**
 * @brief Block until an asynchronous operation completes.
 * @param result The result returned by the async function.
 * @return Final operation status.
 */
hal_sdio_error_t hal_sdio_wait_sync(hal_sdio_error_t result);

/**
 * @brief Read a single 512-byte block from the SD card.
 * @param addr   Sector address (LBA).
 * @param buffer 512-byte destination buffer.
 * @return Read status.
 */
hal_sdio_error_t hal_sdio_read_block(uint32_t addr, uint8_t *buffer);

/**
 * @brief Write a single 512-byte block to the SD card.
 * @param addr   Sector address (LBA).
 * @param buffer 512-byte source buffer.
 * @return Write status.
 */
hal_sdio_error_t hal_sdio_write_block(uint32_t addr, const uint8_t *buffer);

/**
 * @brief Get the SD card's total sector count.
 * @return Number of 512-byte sectors.
 */
uint32_t hal_sdio_get_sector_count(void);

#endif /* NAVHAL_CONFIG_DRV_SDIO */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: register-bit defines, async/DMA prototypes, compat. */
#if NAVHAL_CONFIG_DRV_SDIO
#include "navhal_port_sdio.h"

#if defined(NAVHAL_PORT_SDIO_COMPAT)
#include "compat/sdio_compat.h"
#endif
#endif


/** @} */ /* end of group HAL_SDIO */
#endif /* HAL_SDIO_H */
