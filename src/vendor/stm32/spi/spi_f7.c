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
 * @file spi_f7.c
 * @brief Standardized HAL SPI driver for STM32F7 (Cortex-M7) — SPI1 / SPI2.
 *
 * @details
 * Same `hal_spi_*` contract as the F4 `spi.c`, but for the F7 SPI's two
 * differences (RM0410 §35):
 *  - **Data size** is in `CR2.DS[3:0]` (the F4's `CR1.DFF` bit is gone) — and
 *    `CR2 = 0` (as the F4 driver does) would select an invalid 0-bit frame, so
 *    the size must be set explicitly.
 *  - **RX FIFO**: `CR2.FRXTH = 1` makes `RXNE` assert on an 8-bit boundary, and
 *    the data register must be accessed **byte-wise** to pop one byte from the
 *    FIFO; a 32-bit read of `DR` would drain two frames.
 *
 * Master mode, blocking. The vendor CMakeLists selects this file in place of
 * `spi.c` when `CONFIG_FAMILY_STM32F7` is set.
 */

#include "navhal_port_spi.h"

#include "internal/hal_spi_ops.h"
#include "navhal_port_gpio.h"
#include "family/rcc_reg.h"
#include "family/spi_reg.h"
#include "navhal_port_timer.h"

static inline volatile SPI_Reg_Typedef *_get_spi(hal_spi_instance_t spi) {
  return (volatile SPI_Reg_Typedef *)GET_SPIx_BASE((uint8_t)spi);
}

/** @brief Byte access to the data register (pops/pushes one FIFO frame). */
static inline volatile uint8_t *_spi_dr8(volatile SPI_Reg_Typedef *s) {
  return (volatile uint8_t *)&s->DR;
}

static void _enable_spi_clock(hal_spi_instance_t spi) {
  if (spi == HAL_SPI_1)
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  else if (spi == HAL_SPI_2)
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
}

static void _configure_spi_gpio(hal_spi_instance_t spi) {
  if (spi == HAL_SPI_1) {
    /* SPI1: PA5 SCK / PA6 MISO / PA7 MOSI, AF5. */
    const hal_gpio_pin_t pins[] = {GPIO_PA05, GPIO_PA06, GPIO_PA07};
    for (unsigned i = 0; i < 3; i++) {
      hal_gpio_enable_clock(pins[i]);
      hal_gpio_set_mode(pins[i], HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
      hal_gpio_set_alternate_function(pins[i], HAL_GPIO_AF5);
      hal_gpio_set_output_speed(pins[i], HAL_GPIO_SPEED_VERY_HIGH);
    }
  } else if (spi == HAL_SPI_2) {
    /* SPI2: PB13 SCK / PB14 MISO / PB15 MOSI, AF5. */
    const hal_gpio_pin_t pins[] = {GPIO_PB13, GPIO_PB14, GPIO_PB15};
    for (unsigned i = 0; i < 3; i++) {
      hal_gpio_enable_clock(pins[i]);
      hal_gpio_set_mode(pins[i], HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
      hal_gpio_set_alternate_function(pins[i], HAL_GPIO_AF5);
      hal_gpio_set_output_speed(pins[i], HAL_GPIO_SPEED_VERY_HIGH);
    }
  }
}

static hal_status_t stm32f7_spi_init(hal_spi_instance_t spi,
                          const hal_spi_config_t *config) {
  if (!config)
    return HAL_ERR_INVALID_ARG;

  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg)
    return HAL_ERR_INVALID_ARG;

  _enable_spi_clock(spi);
  _configure_spi_gpio(spi);

  spi_reg->CR1 &= ~SPI_CR1_SPE; /* disable before configuring */

  uint32_t cr1 = SPI_CR1_MSTR | (config->baudrate << SPI_CR1_BR_Pos);
  if (config->cpol == HAL_SPI_CPOL_HIGH)
    cr1 |= SPI_CR1_CPOL;
  if (config->cpha == HAL_SPI_CPHA_2EDGE)
    cr1 |= SPI_CR1_CPHA;
  if (config->firstbit == HAL_SPI_FIRSTBIT_LSB)
    cr1 |= SPI_CR1_LSBFIRST;
  /* Software slave management for single-master. (No CR1.DFF on F7.) */
  cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;
  spi_reg->CR1 = cr1;

  /* CR2: frame size in DS, and FRXTH so RXNE fires on an 8-bit boundary. */
  if (config->datasize == HAL_SPI_DATASIZE_16BIT)
    spi_reg->CR2 = SPI_CR2_DS_16BIT;
  else
    spi_reg->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;

  spi_reg->CR1 |= SPI_CR1_SPE;
  return HAL_OK;
}

/* Spin guard rather than a millisecond timeout: the shared layer documents
 * hal_spi_*'s timeout argument as a no-op, so a bounded spin is what every
 * port now does. Matches the F4 backend. */
#define STM32F7_SPI_XFER_GUARD 0xFFFFu

static hal_status_t stm32f7_spi_xfer_byte(hal_spi_instance_t spi, uint8_t out,
                                          uint8_t *in) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg)
    return HAL_ERR_INVALID_ARG;

  uint16_t guard = STM32F7_SPI_XFER_GUARD;
  while (!(spi_reg->SR & SPI_SR_TXE)) {
    if (--guard == 0u)
      return HAL_ERR_TIMEOUT;
  }
  *_spi_dr8(spi_reg) = out;

  guard = STM32F7_SPI_XFER_GUARD;
  while (!(spi_reg->SR & SPI_SR_RXNE)) {
    if (--guard == 0u)
      return HAL_ERR_TIMEOUT;
  }
  uint8_t r = *_spi_dr8(spi_reg);
  if (in != NULL)
    *in = r;
  return HAL_OK;
}

/** @brief The F7 SPI primitives; framing loops live in the shared layer. */
const hal_spi_ops_t _hal_spi_ops = {
    .init = stm32f7_spi_init,
    .xfer_byte = stm32f7_spi_xfer_byte,
};
