/**
 * @file sdio.h
 * @brief SDIO driver interface for NavHAL (Cortex-M4).
 *
 * Provides high-level functions for SD card communication using the SDIO
 * peripheral. Supports 1-bit and 4-bit bus widths.
 */

#ifndef CORTEX_M4_SDIO_H
#define CORTEX_M4_SDIO_H
#define SDIO_CMD_WAITPEND (1 << 9)
#define SDIO_CMD_CPSMEN (1 << 10)

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

#include "core/cortex-m4/sdio_reg.h"
#include <stdint.h>

/**
 * @brief SDIO initialization structure.
 */
typedef struct {
  uint32_t clock_div; /**< SDIO_CK = SDIOCLK / [clock_div + 2] */
  uint8_t bus_width;  /**< 0: 1-bit, 1: 4-bit */
} hal_sdio_config_t;

/**
 * @brief SDIO Error codes.
 */
typedef enum {
  HAL_SDIO_OK = 0,
  HAL_SDIO_ERROR,
  HAL_SDIO_TIMEOUT,
  HAL_SDIO_CRC_FAIL,
  HAL_SDIO_RX_OVERRUN,
  HAL_SDIO_TX_UNDERRUN
} hal_sdio_error_t;

/**
 * @brief Initialize the SDIO peripheral and GPIOs.
 *
 * Configures PC8-PC11 (DAT0-3), PC12 (CLK), and PD2 (CMD).
 * Enables the SDIO clock.
 *
 * @param config Pointer to configuration settings.
 * @return hal_sdio_error_t Status of operation.
 */
hal_sdio_error_t sdio_init(const hal_sdio_config_t *config);

/**
 * @brief Full initialization sequence for SD card.
 *
 * Performs CMD0, CMD8, ACMD41, CMD2, CMD3 and CMD7 to put card
 * into Transfer State.
 *
 * @return hal_sdio_error_t Status of initialization.
 */
hal_sdio_error_t sdio_card_init(void);

/**
 * @brief Send a command to the SD card.
 *
 * @param cmd_index Command index (0-63).
 * @param argument  Command argument.
 * @param wait_resp Wait response type (None, Short, Long).
 * @return hal_sdio_error_t Status of command transmission.
 */
hal_sdio_error_t sdio_send_command(uint8_t cmd_index, uint32_t argument,
                                   uint32_t wait_resp);

/**
 * @brief Get the response from the last command.
 *
 * @param response_reg Response register index (1-4).
 * @return uint32_t Response value.
 */
uint32_t sdio_get_response(uint8_t response_reg);

/**
 * @brief Wait for the SDIO status flag.
 *
 * @param flag Status flag to wait for.
 * @param timeout Maximum wait time.
 * @return hal_sdio_error_t OK if flag is set, TIMEOUT otherwise.
 */
hal_sdio_error_t sdio_wait_flag(uint32_t flag, uint32_t timeout);

/**
 * @brief Read a single 512-byte block from the SD card.
 *
 * @param addr   Sector address (LBA).
 * @param buffer Pointer to 512-byte destination buffer.
 * @return hal_sdio_error_t Status of read operation.
 */
hal_sdio_error_t sdio_read_block(uint32_t addr, uint8_t *buffer);

/**
 * @brief Write a single 512-byte block to the SD card.
 *
 * @param addr   Sector address (LBA).
 * @param buffer Pointer to 512-byte source buffer.
 * @return hal_sdio_error_t Status of write operation.
 */
hal_sdio_error_t sdio_write_block(uint32_t addr, const uint8_t *buffer);

#include "core/cortex-m4/config.h"
#ifdef _DMA_ENABLED
hal_sdio_error_t sdio_read_block_dma(uint32_t addr, uint8_t *buffer);
hal_sdio_error_t sdio_write_block_dma(uint32_t addr, const uint8_t *buffer);

hal_sdio_error_t sdio_read_blocks_dma(uint32_t addr, uint8_t *buffer,
                                      uint32_t count);
hal_sdio_error_t sdio_write_blocks_dma(uint32_t addr, const uint8_t *buffer,
                                       uint32_t count);
#endif

uint32_t sdio_get_sector_count(void);

#endif // !CORTEX_M4_SDIO_H
