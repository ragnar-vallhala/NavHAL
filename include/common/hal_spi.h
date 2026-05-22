/**
 * @file hal_spi.h
 * @brief Portable HAL interface for SPI communication.
 *
 * @details
 * Standardized SPI API (see @c docs/api_standardization.md). All public
 * functions use the @c hal_spi_ prefix, take the SPI instance first, and
 * return ::hal_status_t. Master-mode, polling-mode operation.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/spi_types.h" /* port-resolved ::hal_spi_instance_t enum */
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI baud-rate prescaler.
 */
typedef enum {
  HAL_SPI_BAUDRATE_DIV2 = 0,
  HAL_SPI_BAUDRATE_DIV4 = 1,
  HAL_SPI_BAUDRATE_DIV8 = 2,
  HAL_SPI_BAUDRATE_DIV16 = 3,
  HAL_SPI_BAUDRATE_DIV32 = 4,
  HAL_SPI_BAUDRATE_DIV64 = 5,
  HAL_SPI_BAUDRATE_DIV128 = 6,
  HAL_SPI_BAUDRATE_DIV256 = 7,
  SPI_BAUDRATE_DIV2 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV2") = 0,
  SPI_BAUDRATE_DIV4 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV4") = 1,
  SPI_BAUDRATE_DIV8 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV8") = 2,
  SPI_BAUDRATE_DIV16 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV16") = 3,
  SPI_BAUDRATE_DIV32 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV32") = 4,
  SPI_BAUDRATE_DIV64 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV64") = 5,
  SPI_BAUDRATE_DIV128 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV128") = 6,
  SPI_BAUDRATE_DIV256 NAVHAL_DEPRECATED("use HAL_SPI_BAUDRATE_DIV256") = 7,
} hal_spi_baudrate_t;

/**
 * @brief SPI clock polarity.
 */
typedef enum {
  HAL_SPI_CPOL_LOW = 0,
  HAL_SPI_CPOL_HIGH = 1,
  SPI_CPOL_LOW NAVHAL_DEPRECATED("use HAL_SPI_CPOL_LOW") = 0,
  SPI_CPOL_HIGH NAVHAL_DEPRECATED("use HAL_SPI_CPOL_HIGH") = 1,
} hal_spi_cpol_t;

/**
 * @brief SPI clock phase.
 */
typedef enum {
  HAL_SPI_CPHA_1EDGE = 0,
  HAL_SPI_CPHA_2EDGE = 1,
  SPI_CPHA_1EDGE NAVHAL_DEPRECATED("use HAL_SPI_CPHA_1EDGE") = 0,
  SPI_CPHA_2EDGE NAVHAL_DEPRECATED("use HAL_SPI_CPHA_2EDGE") = 1,
} hal_spi_cpha_t;

/**
 * @brief SPI data size.
 */
typedef enum {
  HAL_SPI_DATASIZE_8BIT = 0,
  HAL_SPI_DATASIZE_16BIT = 1,
  SPI_DATASIZE_8BIT NAVHAL_DEPRECATED("use HAL_SPI_DATASIZE_8BIT") = 0,
  SPI_DATASIZE_16BIT NAVHAL_DEPRECATED("use HAL_SPI_DATASIZE_16BIT") = 1,
} hal_spi_datasize_t;

/**
 * @brief SPI first-bit ordering.
 */
typedef enum {
  HAL_SPI_FIRSTBIT_MSB = 0,
  HAL_SPI_FIRSTBIT_LSB = 1,
  SPI_FIRSTBIT_MSB NAVHAL_DEPRECATED("use HAL_SPI_FIRSTBIT_MSB") = 0,
  SPI_FIRSTBIT_LSB NAVHAL_DEPRECATED("use HAL_SPI_FIRSTBIT_LSB") = 1,
} hal_spi_firstbit_t;

/**
 * @brief SPI configuration structure.
 */
typedef struct {
  hal_spi_baudrate_t baudrate; /**< Baud-rate prescaler. */
  hal_spi_cpol_t cpol;         /**< Clock polarity. */
  hal_spi_cpha_t cpha;         /**< Clock phase. */
  hal_spi_datasize_t datasize; /**< Frame data size. */
  hal_spi_firstbit_t firstbit; /**< Bit ordering. */
} hal_spi_config_t;

/**
 * @brief Initialize an SPI peripheral in master mode.
 * @param spi    SPI instance.
 * @param config Configuration; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL config / invalid SPI.
 */
hal_status_t hal_spi_init(hal_spi_instance_t spi,
                          const hal_spi_config_t *config);

/**
 * @brief Transmit data over SPI (blocking).
 * @param spi     SPI instance.
 * @param data    Source buffer.
 * @param size    Number of frames to transmit.
 * @param timeout Timeout in milliseconds (0 = infinite).
 * @return ::HAL_OK, ::HAL_ERR_TIMEOUT, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_spi_transmit(hal_spi_instance_t spi, const uint8_t *data,
                              uint16_t size, uint32_t timeout);

/**
 * @brief Receive data over SPI (blocking).
 * @param spi     SPI instance.
 * @param data    Destination buffer.
 * @param size    Number of frames to receive.
 * @param timeout Timeout in milliseconds (0 = infinite).
 * @return ::HAL_OK, ::HAL_ERR_TIMEOUT, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                             uint16_t size, uint32_t timeout);

/**
 * @brief Full-duplex transmit + receive over SPI (blocking).
 * @param spi     SPI instance.
 * @param tx_data Source buffer.
 * @param rx_data Destination buffer.
 * @param size    Number of frames to transfer.
 * @param timeout Timeout in milliseconds (0 = infinite).
 * @return ::HAL_OK, ::HAL_ERR_TIMEOUT, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                      const uint8_t *tx_data, uint8_t *rx_data,
                                      uint16_t size, uint32_t timeout);

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization SPI status type and codes. The status enum
 * is now ::hal_status_t; these aliases are retained as a backward-compat
 * surface behind NAVHAL_DEPRECATED.
 * -------------------------------------------------------------------------- */
typedef hal_status_t hal_spi_status_t NAVHAL_DEPRECATED("use hal_status_t");
#define HAL_SPI_OK HAL_OK                    /**< @deprecated HAL_OK */
#define HAL_SPI_ERR_TIMEOUT HAL_ERR_TIMEOUT  /**< @deprecated HAL_ERR_TIMEOUT */
#define HAL_SPI_ERR_BUSY HAL_ERR_BUSY        /**< @deprecated HAL_ERR_BUSY */
#define HAL_SPI_ERR_PARAM HAL_ERR_INVALID_ARG /**< @deprecated HAL_ERR_INVALID_ARG */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits (currently none for SPI; header retained for the
 * existing @c #include "navhal_port_spi.h" path). */
#include "navhal_port_spi.h"

#endif /* HAL_SPI_H */
