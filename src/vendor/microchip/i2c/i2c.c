/**
 * @file src/vendor/microchip/i2c/i2c.c
 * @brief ATmega328P I²C (TWI) HAL driver — blocking master mode.
 *
 * @details
 * Implements @c common/hal_i2c.h on the ATmega328P's single TWI peripheral,
 * exposed as ::HAL_I2C_0. Master mode only; transfers are blocking and
 * polled, with a coarse iteration-count timeout on each TWI wait so a
 * missing or stuck device cannot hang the caller forever.
 *
 * TWI uses the fixed pins PC5 (SCL) and PC4 (SDA); their internal pull-ups
 * are enabled here, though a real bus still needs external pull-ups.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_i2c.h"

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Set once ::HAL_I2C_0 has been initialised (bit 0). */
static uint8_t s_init_mask = 0;

/** @brief TWI status code, masked to the significant bits. */
#define TWI_STATUS() (TWSR & 0xF8u)

/** @brief Spin until the TWINT flag sets; false on timeout. */
static bool twi_wait(void) {
  uint16_t guard = 0;
  while (!(TWCR & (1u << TWINT))) {
    if (++guard == 0u)
      return false;
  }
  return true;
}

/** @brief Issue a START (or repeated START) condition. */
static bool twi_start(void) {
  TWCR = (uint8_t)((1u << TWINT) | (1u << TWSTA) | (1u << TWEN));
  if (!twi_wait())
    return false;
  uint8_t st = TWI_STATUS();
  return (st == 0x08u || st == 0x10u);
}

/** @brief Issue a STOP condition. */
static void twi_stop(void) {
  TWCR = (uint8_t)((1u << TWINT) | (1u << TWSTO) | (1u << TWEN));
}

/** @brief Transmit one byte; @p expect is the success status code. */
static bool twi_write(uint8_t byte, uint8_t expect) {
  TWDR = byte;
  TWCR = (uint8_t)((1u << TWINT) | (1u << TWEN));
  if (!twi_wait())
    return false;
  return TWI_STATUS() == expect;
}

/** @brief Receive one byte, ACKing it when @p ack is true. */
static bool twi_read(uint8_t *out, bool ack) {
  TWCR = (uint8_t)((1u << TWINT) | (1u << TWEN) | (ack ? (1u << TWEA) : 0u));
  if (!twi_wait())
    return false;
  *out = TWDR;
  return true;
}

hal_status_t hal_i2c_init(hal_i2c_bus_t bus, const hal_i2c_config_t *config) {
  if (bus != HAL_I2C_0 || config == NULL)
    return HAL_ERR_INVALID_ARG;
  if (config->own_address != I2C_MASTER)
    return HAL_ERR_IO; /* slave mode is not supported. */
  if (s_init_mask & 0x01u)
    return HAL_ERR_NOT_INITIALIZED; /* re-init is rejected by contract. */

  /* SCL = F_CPU / (16 + 2*TWBR*prescaler), prescaler = 1.
   *   100 kHz @ 16 MHz -> TWBR 72;  400 kHz -> TWBR 12. */
  TWSR = 0; /* prescaler = 1. */
  TWBR = (config->clock_speed == HAL_I2C_SPEED_FAST) ? 12u : 72u;

  /* Enable the internal pull-ups on the TWI pins (PC4 = SDA, PC5 = SCL). */
  PORTC |= (uint8_t)((1u << PC4) | (1u << PC5));

  TWCR = (uint8_t)(1u << TWEN);
  s_init_mask |= 0x01u;
  return HAL_OK;
}

hal_status_t hal_i2c_write(hal_i2c_bus_t bus, uint8_t dev_addr,
                           const uint8_t *data, uint16_t len) {
  if (bus != HAL_I2C_0 || data == NULL)
    return HAL_ERR_INVALID_ARG;
  if (!twi_start()) {
    twi_stop();
    return HAL_ERR_TIMEOUT;
  }
  /* SLA+W; 0x18 = address ACKed. */
  if (!twi_write((uint8_t)(dev_addr << 1), 0x18u)) {
    twi_stop();
    return HAL_ERR_IO;
  }
  for (uint16_t i = 0; i < len; i++) {
    /* 0x28 = data byte ACKed. */
    if (!twi_write(data[i], 0x28u)) {
      twi_stop();
      return HAL_ERR_IO;
    }
  }
  twi_stop();
  return HAL_OK;
}

hal_status_t hal_i2c_read(hal_i2c_bus_t bus, uint8_t dev_addr, uint8_t *data,
                          uint16_t len) {
  if (bus != HAL_I2C_0 || data == NULL)
    return HAL_ERR_INVALID_ARG;
  if (!twi_start()) {
    twi_stop();
    return HAL_ERR_TIMEOUT;
  }
  /* SLA+R; 0x40 = address ACKed. */
  if (!twi_write((uint8_t)((dev_addr << 1) | 1u), 0x40u)) {
    twi_stop();
    return HAL_ERR_IO;
  }
  for (uint16_t i = 0; i < len; i++) {
    /* ACK every byte except the last, which is NACKed to end the read. */
    if (!twi_read(&data[i], i + 1u < len)) {
      twi_stop();
      return HAL_ERR_TIMEOUT;
    }
  }
  twi_stop();
  return HAL_OK;
}

hal_status_t hal_i2c_write_read(hal_i2c_bus_t bus, uint8_t dev_addr,
                                const uint8_t *tx_data, uint16_t tx_len,
                                uint8_t *rx_data, uint16_t rx_len) {
  if (bus != HAL_I2C_0 || tx_data == NULL || rx_data == NULL)
    return HAL_ERR_INVALID_ARG;

  /* Write phase. */
  if (!twi_start()) {
    twi_stop();
    return HAL_ERR_TIMEOUT;
  }
  if (!twi_write((uint8_t)(dev_addr << 1), 0x18u)) {
    twi_stop();
    return HAL_ERR_IO;
  }
  for (uint16_t i = 0; i < tx_len; i++) {
    if (!twi_write(tx_data[i], 0x28u)) {
      twi_stop();
      return HAL_ERR_IO;
    }
  }

  /* Repeated START + read phase. */
  if (!twi_start()) {
    twi_stop();
    return HAL_ERR_TIMEOUT;
  }
  if (!twi_write((uint8_t)((dev_addr << 1) | 1u), 0x40u)) {
    twi_stop();
    return HAL_ERR_IO;
  }
  for (uint16_t i = 0; i < rx_len; i++) {
    if (!twi_read(&rx_data[i], i + 1u < rx_len)) {
      twi_stop();
      return HAL_ERR_TIMEOUT;
    }
  }
  twi_stop();
  return HAL_OK;
}

uint8_t hal_i2c_get_init_status(void) { return s_init_mask; }
