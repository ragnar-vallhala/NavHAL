/**
 * @file hal_sdio.h
 * @brief Portable HAL interface for SDIO.
 *
 * @details
 * Standardized SDIO API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_sdio_ prefix. Supports 1-bit and 4-bit bus widths
 * and (when the port's DMA backend is enabled) asynchronous block transfers.
 *
 * The entire API is compiled only when @c _SDIO_ENABLED is defined (see
 * ::NAVHAL_HAS_SDIO) — on a target without an SDIO peripheral the header
 * collapses to nothing, exactly as @c hal_dma.h does for DMA.
 *
 * @note SDIO returns the driver-specific ::hal_sdio_error_t rather than
 *       ::hal_status_t — its asynchronous model needs ::HAL_SDIO_PENDING,
 *       which the standard status enum cannot express. Flagged for the M5
 *       conformance review.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_SDIO_H
#define HAL_SDIO_H

#include "common/hal_config.h" /* sources the _SDIO_ENABLED capability flag */
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _SDIO_ENABLED

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
 * @brief SDIO initialization configuration.
 */
typedef struct {
  uint32_t clock_div; /**< SDIO_CK = SDIOCLK / (clock_div + 2). */
  uint8_t bus_width;  /**< 0: 1-bit, 1: 4-bit. */
} hal_sdio_config_t;

/**
 * @brief SDIO operation status / error codes.
 */
typedef enum {
  HAL_SDIO_OK = 0,
  HAL_SDIO_ERROR,
  HAL_SDIO_TIMEOUT,
  HAL_SDIO_CRC_FAIL,
  HAL_SDIO_RX_OVERRUN,
  HAL_SDIO_TX_UNDERRUN,
  HAL_SDIO_PENDING,
  HAL_SDIO_BUSY
} hal_sdio_error_t;

/**
 * @brief SDIO completion callback type for asynchronous operations.
 */
typedef void (*hal_sdio_callback_t)(hal_sdio_error_t error);

/**
 * @brief Set the callback for asynchronous SDIO operations.
 * @param callback Function invoked when an async operation completes.
 */
void hal_sdio_set_callback(hal_sdio_callback_t callback);

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

#endif /* _SDIO_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits: register-bit defines, async/DMA prototypes, compat. */
#include "navhal_port_sdio.h"

#endif /* HAL_SDIO_H */
