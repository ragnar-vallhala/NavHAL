/**
 * @file src/vendor/microchip/spi/spi.c
 * @brief ATmega328P SPI HAL driver — blocking master mode.
 *
 * @details
 * Implements @c common/hal_spi.h on the ATmega328P's single SPI peripheral,
 * exposed as ::HAL_SPI_0. Master mode, 8-bit frames, blocking transfers.
 *
 * SPI uses fixed pins: SCK = PB5, MISO = PB4, MOSI = PB3, SS = PB2. SS is
 * driven as an output so the peripheral cannot be demoted to slave mode.
 * The ATmega328P SPI unit is 8-bit only, so ::HAL_SPI_DATASIZE_16BIT is
 * treated as 8-bit. Its slowest clock is F_CPU/128, so
 * ::HAL_SPI_BAUDRATE_DIV256 is clamped to /128.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_spi.h"

#include <avr/io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SPR1:0 and SPI2X for each hal_spi_baudrate_t (DIV2..DIV256). DIV256 has
 * no AVR equivalent and clamps to /128. */
static const uint8_t k_spr[8] = {0, 0, 1, 1, 2, 2, 3, 3};
static const uint8_t k_spi2x[8] = {1, 0, 1, 0, 1, 0, 0, 0};

/** @brief Exchange one byte; false on a (guarded) timeout. */
static bool spi_xfer(uint8_t out, uint8_t *in) {
  SPDR = out;
  uint16_t guard = 0;
  while (!(SPSR & (1u << SPIF))) {
    if (++guard == 0u)
      return false;
  }
  uint8_t r = SPDR;
  if (in != NULL)
    *in = r;
  return true;
}

hal_status_t hal_spi_init(hal_spi_instance_t spi,
                          const hal_spi_config_t *config) {
  if (spi != HAL_SPI_0 || config == NULL)
    return HAL_ERR_INVALID_ARG;

  uint8_t baud = (uint8_t)config->baudrate;
  if (baud > 7u)
    return HAL_ERR_INVALID_ARG;

  /* MOSI (PB3), SCK (PB5), SS (PB2) outputs; MISO (PB4) input. */
  DDRB |= (uint8_t)((1u << PB3) | (1u << PB5) | (1u << PB2));
  DDRB &= (uint8_t)~(1u << PB4);

  uint8_t spcr = (uint8_t)((1u << SPE) | (1u << MSTR));
  if (config->cpol == HAL_SPI_CPOL_HIGH)
    spcr |= (uint8_t)(1u << CPOL);
  if (config->cpha == HAL_SPI_CPHA_2EDGE)
    spcr |= (uint8_t)(1u << CPHA);
  if (config->firstbit == HAL_SPI_FIRSTBIT_LSB)
    spcr |= (uint8_t)(1u << DORD);
  spcr |= (uint8_t)(k_spr[baud] & 0x03u);
  SPCR = spcr;

  if (k_spi2x[baud])
    SPSR |= (uint8_t)(1u << SPI2X);
  else
    SPSR &= (uint8_t)~(1u << SPI2X);

  return HAL_OK;
}

hal_status_t hal_spi_transmit(hal_spi_instance_t spi, const uint8_t *data,
                              uint16_t size, uint32_t timeout) {
  (void)timeout; /* AVR SPI uses a coarse iteration guard, not a ms timeout. */
  if (spi != HAL_SPI_0 || data == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < size; i++) {
    if (!spi_xfer(data[i], NULL))
      return HAL_ERR_TIMEOUT;
  }
  return HAL_OK;
}

hal_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                             uint16_t size, uint32_t timeout) {
  (void)timeout;
  if (spi != HAL_SPI_0 || data == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < size; i++) {
    if (!spi_xfer(0xFFu, &data[i])) /* clock out a dummy frame to read. */
      return HAL_ERR_TIMEOUT;
  }
  return HAL_OK;
}

hal_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                      const uint8_t *tx_data, uint8_t *rx_data,
                                      uint16_t size, uint32_t timeout) {
  (void)timeout;
  if (spi != HAL_SPI_0 || tx_data == NULL || rx_data == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < size; i++) {
    if (!spi_xfer(tx_data[i], &rx_data[i]))
      return HAL_ERR_TIMEOUT;
  }
  return HAL_OK;
}
