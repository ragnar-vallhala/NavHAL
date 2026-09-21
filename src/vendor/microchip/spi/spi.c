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
 */

#include "internal/hal_spi_ops.h"

#include <avr/io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SPR1:0 and SPI2X for each hal_spi_baudrate_t (DIV2..DIV256). DIV256 has
 * no AVR equivalent and clamps to /128. */
static const uint8_t k_spr[8] = {0, 0, 1, 1, 2, 2, 3, 3};
static const uint8_t k_spi2x[8] = {1, 0, 1, 0, 1, 0, 0, 0};

static hal_status_t avr_spi_xfer_byte(hal_spi_instance_t spi, uint8_t out,
                                      uint8_t *in) {
  if (spi != HAL_SPI_0)
    return HAL_ERR_INVALID_ARG;
  SPDR = out;
  uint16_t guard = 0;
  while (!(SPSR & (1u << SPIF))) {
    if (++guard == 0u)
      return HAL_ERR_TIMEOUT;
  }
  uint8_t r = SPDR;
  if (in != NULL)
    *in = r;
  return HAL_OK;
}

static hal_status_t avr_spi_init(hal_spi_instance_t spi,
                                 const hal_spi_config_t *config) {
  /* config non-NULL: validated by the public layer. */
  if (spi != HAL_SPI_0)
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

const hal_spi_ops_t _hal_spi_ops = {
    .init = avr_spi_init,
    .xfer_byte = avr_spi_xfer_byte,
};
