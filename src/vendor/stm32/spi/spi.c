/**
 * @file spi.c
 * @brief Standardized HAL SPI driver for STM32F4 (Cortex-M4) — SPI1 and SPI2.
 *
 * @details
 * Implements the standardized `hal_spi_*` API declared in
 * `port/cortex-m4/navhal_port_spi.h`: master-mode initialization and blocking
 * transmit / receive / full-duplex transfers.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_spi.h"
#include "navhal_port_gpio.h"
#include "family/rcc_reg.h"
#include "family/spi_reg.h"
#include "navhal_port_timer.h"

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

static void _configure_spi_gpio(hal_spi_instance_t spi) {
  if (spi == HAL_SPI_1) {
    // SPI1 default pins: PA5 (SCK), PA6 (MISO), PA7 (MOSI)
    hal_gpio_enable_clock(GPIO_PA05);
    hal_gpio_set_mode(GPIO_PA05, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PA05, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PA05, HAL_GPIO_SPEED_VERY_HIGH);

    hal_gpio_enable_clock(GPIO_PA06);
    hal_gpio_set_mode(GPIO_PA06, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PA06, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PA06, HAL_GPIO_SPEED_VERY_HIGH);

    hal_gpio_enable_clock(GPIO_PA07);
    hal_gpio_set_mode(GPIO_PA07, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PA07, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PA07, HAL_GPIO_SPEED_VERY_HIGH);
  } else if (spi == HAL_SPI_2) {
    // SPI2 default pins: PB13 (SCK), PB14 (MISO), PB15 (MOSI)
    hal_gpio_enable_clock(GPIO_PB13);
    hal_gpio_set_mode(GPIO_PB13, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PB13, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PB13, HAL_GPIO_SPEED_VERY_HIGH);

    hal_gpio_enable_clock(GPIO_PB14);
    hal_gpio_set_mode(GPIO_PB14, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PB14, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PB14, HAL_GPIO_SPEED_VERY_HIGH);

    hal_gpio_enable_clock(GPIO_PB15);
    hal_gpio_set_mode(GPIO_PB15, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
    hal_gpio_set_alternate_function(GPIO_PB15, HAL_GPIO_AF5);
    hal_gpio_set_output_speed(GPIO_PB15, HAL_GPIO_SPEED_VERY_HIGH);
  }
}

hal_status_t hal_spi_init(hal_spi_instance_t spi,
                          const hal_spi_config_t *config) {
  if (!config)
    return HAL_ERR_INVALID_ARG;

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

hal_status_t hal_spi_transmit(hal_spi_instance_t spi, const uint8_t *data,
                              uint16_t size, uint32_t timeout) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg || !data)
    return HAL_ERR_INVALID_ARG;

  uint32_t start_tick = hal_timebase_get_millis();

  for (uint16_t i = 0; i < size; i++) {
    // Wait for TXE
    while (!(spi_reg->SR & SPI_SR_TXE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }

    spi_reg->DR = data[i];

    // Optional: Wait for RXNE and read dummy data to clear it
    while (!(spi_reg->SR & SPI_SR_RXNE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }
    (void)spi_reg->DR;
  }

  // Wait for BSY to clear
  while (spi_reg->SR & SPI_SR_BSY) {
    if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
      return HAL_ERR_TIMEOUT;
  }

  return HAL_OK;
}

hal_status_t hal_spi_receive(hal_spi_instance_t spi, uint8_t *data,
                             uint16_t size, uint32_t timeout) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg || !data)
    return HAL_ERR_INVALID_ARG;

  uint32_t start_tick = hal_timebase_get_millis();

  for (uint16_t i = 0; i < size; i++) {
    // Send dummy data to trigger clock
    while (!(spi_reg->SR & SPI_SR_TXE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }
    spi_reg->DR = 0xFF;

    // Wait for RXNE
    while (!(spi_reg->SR & SPI_SR_RXNE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }
    data[i] = (uint8_t)spi_reg->DR;
  }

  return HAL_OK;
}

hal_status_t hal_spi_transmit_receive(hal_spi_instance_t spi,
                                      const uint8_t *tx_data, uint8_t *rx_data,
                                      uint16_t size, uint32_t timeout) {
  volatile SPI_Reg_Typedef *spi_reg = _get_spi(spi);
  if (!spi_reg || !tx_data || !rx_data)
    return HAL_ERR_INVALID_ARG;

  uint32_t start_tick = hal_timebase_get_millis();

  for (uint16_t i = 0; i < size; i++) {
    // Wait for TXE
    while (!(spi_reg->SR & SPI_SR_TXE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }
    spi_reg->DR = tx_data[i];

    // Wait for RXNE
    while (!(spi_reg->SR & SPI_SR_RXNE)) {
      if (timeout && (hal_timebase_get_millis() - start_tick > timeout))
        return HAL_ERR_TIMEOUT;
    }
    rx_data[i] = (uint8_t)spi_reg->DR;
  }

  return HAL_OK;
}
