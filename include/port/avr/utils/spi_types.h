/**
 * @file spi_types.h
 * @brief SPI peripheral-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single SPI peripheral, exposed as ::HAL_SPI_0.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef SPI_TYPES_H
#define SPI_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief SPI peripheral instance identifier (ATmega328P: the SPI unit). */
typedef enum {
  HAL_SPI_0 = 0, /**< SPI — the only SPI peripheral on the ATmega328P. */
} hal_spi_instance_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SPI_TYPES_H */
