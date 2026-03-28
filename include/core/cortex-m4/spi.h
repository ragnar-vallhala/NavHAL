/**
 * @file core/cortex-m4/spi.h
 * @brief Cortex-M4 SPI HAL interface.
 *
 * @details
 * This header defines types, structures, and function prototypes for
 * SPI communication on Cortex-M4 microcontrollers.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_SPI_H
#define CORTEX_M4_SPI_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief SPI operation status codes.
 */
typedef enum {
  HAL_SPI_OK = 0,      /**< Operation successful */
  HAL_SPI_ERR_TIMEOUT, /**< Timeout occurred */
  HAL_SPI_ERR_BUSY,    /**< Peripheral busy */
  HAL_SPI_ERR_PARAM    /**< Invalid parameter */
} hal_spi_status_t;

/**
 * @brief Supported SPI instances.
 */
typedef enum {
  SPI_1 = 1, /**< SPI instance 1 */
  SPI_2 = 2  /**< SPI instance 2 */
} hal_spi_instance_t;

/**
 * @brief SPI BaudRate Prescaler.
 */
typedef enum {
  SPI_BAUDRATE_DIV2 = 0,
  SPI_BAUDRATE_DIV4 = 1,
  SPI_BAUDRATE_DIV8 = 2,
  SPI_BAUDRATE_DIV16 = 3,
  SPI_BAUDRATE_DIV32 = 4,
  SPI_BAUDRATE_DIV64 = 5,
  SPI_BAUDRATE_DIV128 = 6,
  SPI_BAUDRATE_DIV256 = 7
} hal_spi_baudrate_t;

/**
 * @brief SPI Clock Polarity.
 */
typedef enum { SPI_CPOL_LOW = 0, SPI_CPOL_HIGH = 1 } hal_spi_cpol_t;

/**
 * @brief SPI Clock Phase.
 */
typedef enum { SPI_CPHA_1EDGE = 0, SPI_CPHA_2EDGE = 1 } hal_spi_cpha_t;

/**
 * @brief SPI Data Size.
 */
typedef enum {
  SPI_DATASIZE_8BIT = 0,
  SPI_DATASIZE_16BIT = 1
} hal_spi_datasize_t;

/**
 * @brief SPI First Bit.
 */
typedef enum { SPI_FIRSTBIT_MSB = 0, SPI_FIRSTBIT_LSB = 1 } hal_spi_firstbit_t;

/**
 * @brief SPI configuration structure.
 */
typedef struct {
  hal_spi_baudrate_t baudrate;
  hal_spi_cpol_t cpol;
  hal_spi_cpha_t cpha;
  hal_spi_datasize_t datasize;
  hal_spi_firstbit_t firstbit;
} hal_spi_config_t;

/**
 * @brief Initialize the SPI peripheral.
 *
 * @param spi The SPI instance to initialize.
 * @param config Pointer to the configuration structure.
 * @return Status code of the initialization operation.
 */
hal_spi_status_t hal_spi_init(hal_spi_instance_t spi,
                              const hal_spi_config_t *config);

/**
 * @brief Transmit data over SPI.
 *
 * @param spi The SPI instance.
 * @param data Pointer to data buffer.
 * @param size Number of bytes/half-words to transmit.
 * @param timeout Timeout in milliseconds (0 for infinite).
 * @return Status code of the transmit operation.
 */
hal_spi_status_t hal_spi_transmit(hal_spi_instance_t spi, const uint8_t *data,
                                  uint16_t size, uint32_t timeout);

/**
 * @brief Receive data over SPI.
 *
 * @param spi The SPI instance.
 * @param data Pointer to buffer for received data.
 * @param size Number of bytes/half-words to receive.
 * @param timeout Timeout in milliseconds (0 for infinite).
 * @return Status code of the receive operation.
 */
hal_spi_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                                 uint16_t size, uint32_t timeout);

/**
 * @brief Full-duplex transmit and receive data over SPI.
 *
 * @param spi The SPI instance.
 * @param tx_data Pointer to data buffer to transmit.
 * @param rx_data Pointer to buffer for received data.
 * @param size Number of bytes/half-words to transfer.
 * @param timeout Timeout in milliseconds (0 for infinite).
 * @return Status code of the transfer operation.
 */
hal_spi_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                          const uint8_t *tx_data,
                                          uint8_t *rx_data, uint16_t size,
                                          uint32_t timeout);

#endif // CORTEX_M4_SPI_H
