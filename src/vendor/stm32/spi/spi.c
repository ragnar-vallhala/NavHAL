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
 * @file spi.c
 * @brief Standardized HAL SPI driver for STM32F4 (Cortex-M4) — SPI1 and SPI2.
 *
 * @details
 * Implements the standardized `hal_spi_*` API declared in
 * `port/cortex-m4/navhal_port_spi.h`: master-mode initialization and blocking
 * transmit / receive / full-duplex transfers.
 */

#include "internal/hal_spi_ops.h"
#include "common/hal_clock.h"
#include "navhal_port_spi.h"
#include "board.h"
#include "common/hal_gpio.h"
#include "navhal_port_gpio.h"
#include "family/rcc_reg.h"
#include "family/spi_reg.h"

static inline volatile SPI_Reg_Typedef *_get_spi(hal_spi_instance_t spi) {
  return (volatile SPI_Reg_Typedef *)GET_SPIx_BASE((uint8_t)spi);
}

static void _enable_spi_clock(hal_spi_instance_t spi) {
  if (spi == HAL_SPI_1) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  } else if (spi == HAL_SPI_2) {
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
  }
}

/* SPI pin routing, supplied by the board's Kconfig via the generated board.h.
 * An instance the board does not describe is absent from this table, and
 * _configure_spi_gpio then touches no pins for it. */
typedef struct {
  hal_spi_instance_t spi;
  hal_gpio_pin_t sck;
  hal_gpio_pin_t miso;
  hal_gpio_pin_t mosi;
  hal_gpio_af_t af;
} spi_pinmap_t;

static const spi_pinmap_t spi_pinmap[] = {
#if defined(BOARD_SPI1_SCK)
    {HAL_SPI_1, BOARD_SPI1_SCK, BOARD_SPI1_MISO, BOARD_SPI1_MOSI,
     (hal_gpio_af_t)BOARD_SPI1_AF},
#endif
#if defined(BOARD_SPI2_SCK)
    {HAL_SPI_2, BOARD_SPI2_SCK, BOARD_SPI2_MISO, BOARD_SPI2_MOSI,
     (hal_gpio_af_t)BOARD_SPI2_AF},
#endif
};

static void _configure_spi_gpio(hal_spi_instance_t spi) {
  for (unsigned i = 0u; i < (sizeof spi_pinmap / sizeof spi_pinmap[0]); i++) {
    if (spi_pinmap[i].spi != spi)
      continue;
    const hal_gpio_pin_t pins[3] = {spi_pinmap[i].sck, spi_pinmap[i].miso,
                                    spi_pinmap[i].mosi};
    for (unsigned p = 0u; p < 3u; p++) {
      hal_gpio_enable_clock(pins[p]);
      hal_gpio_set_mode(pins[p], HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
      hal_gpio_set_alternate_function(pins[p], spi_pinmap[i].af);
      hal_gpio_set_output_speed(pins[p], HAL_GPIO_SPEED_VERY_HIGH);
    }
    return;
  }
}

static hal_status_t stm32_spi_init(hal_spi_instance_t spi,
                                   const hal_spi_config_t *config) {
  /* config non-NULL: validated by the public layer. */
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg)
    return HAL_ERR_INVALID_ARG;

  _enable_spi_clock(spi);
  _configure_spi_gpio(spi);

  // Disable SPI before configuration
  spi_reg->CR1 &= ~SPI_CR1_SPE;

  uint32_t cr1 = 0;

  // Master mode
  cr1 |= SPI_CR1_MSTR;

  // Baudrate
  cr1 |= (config->baudrate << SPI_CR1_BR_Pos);

  // CPOL/CPHA
  if (config->cpol == HAL_SPI_CPOL_HIGH)
    cr1 |= SPI_CR1_CPOL;
  if (config->cpha == HAL_SPI_CPHA_2EDGE)
    cr1 |= SPI_CR1_CPHA;

  // Data size
  if (config->datasize == HAL_SPI_DATASIZE_16BIT)
    cr1 |= SPI_CR1_DFF;

  // First bit
  if (config->firstbit == HAL_SPI_FIRSTBIT_LSB)
    cr1 |= SPI_CR1_LSBFIRST;

  // Software Slave Management (SSM=1, SSI=1) - typically used for single master
  cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;

  spi_reg->CR1 = cr1;
  spi_reg->CR2 = 0; // Standard configuration

  // Enable SPI
  spi_reg->CR1 |= SPI_CR1_SPE;

  return HAL_OK;
}

/* Bound a stuck flag-wait without a millisecond time source: ~65k spins is
 * far longer than one byte at any SPI clock, but still terminates. */
#define STM32_SPI_XFER_GUARD 0xFFFFu

static hal_status_t stm32_spi_xfer_byte(hal_spi_instance_t spi, uint8_t out,
                                        uint8_t *in) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg)
    return HAL_ERR_INVALID_ARG;

  uint16_t guard = STM32_SPI_XFER_GUARD;
  while (!(spi_reg->SR & SPI_SR_TXE)) {
    if (--guard == 0u)
      return HAL_ERR_TIMEOUT;
  }
  spi_reg->DR = out;

  guard = STM32_SPI_XFER_GUARD;
  while (!(spi_reg->SR & SPI_SR_RXNE)) {
    if (--guard == 0u)
      return HAL_ERR_TIMEOUT;
  }
  uint8_t r = (uint8_t)spi_reg->DR;
  if (in != NULL)
    *in = r;
  return HAL_OK;
}


/* SPI1 is an APB2 peripheral; SPI2 (and SPI3) hang off APB1. */
static uint32_t stm32_spi_input_clock(hal_spi_instance_t spi) {
  return (spi == HAL_SPI_1) ? hal_clock_get_apb2clk() : hal_clock_get_apb1clk();
}

static uint8_t stm32_spi_get_baudrate(hal_spi_instance_t spi) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg)
    return 0u;
  return (uint8_t)((spi_reg->CR1 & SPI_CR1_BR_Msk) >> SPI_CR1_BR_Pos);
}

const hal_spi_ops_t _hal_spi_ops = {
    .init = stm32_spi_init,
    .xfer_byte = stm32_spi_xfer_byte,
    .input_clock = stm32_spi_input_clock,
    .get_baudrate = stm32_spi_get_baudrate,
};
