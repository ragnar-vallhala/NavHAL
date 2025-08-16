/**
 * @file i2c.c
 * @brief HAL I2C driver implementation for STM32F4 series
 * @details
 * This file provides a complete hardware abstraction layer for I2C communication
 * on STM32F4 microcontrollers. It supports:
 * - Master mode operation (7-bit addressing)
 * - Standard mode (100 kHz) and Fast mode (400 kHz)
 * - Combined write/read operations
 * - Timeout handling for robust operation
 * - Automatic clock configuration
 *
 * The implementation handles all low-level register operations and provides
 * high-level functions for common I2C transactions.
 *
 * @note Slave mode is not currently implemented (marked as TODO)
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/i2c.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/i2c_reg.h"
#include "core/cortex-m4/rcc_reg.h"
#include <stdbool.h>
#include <stdint.h>

/* Private defines */
#define TIMEOUT 1000000          ///< Maximum wait cycles for flag checks
#define I2C1_EN 1                ///< Init status mask for I2C1
#define I2C2_EN 2                ///< Init status mask for I2C2
#define I2C3_EN 4                ///< Init status mask for I2C3

static uint8_t __i2c_init_status = 0; ///< Tracks initialization status of each bus

/**
 * @brief Get initialization status of I2C buses
 * @return Bitmask indicating which I2C buses are initialized
 * @retval Bit 0: I2C1 status
 * @retval Bit 1: I2C2 status
 * @retval Bit 2: I2C3 status
 */
uint8_t hal_i2c_get_init_status(void) { return __i2c_init_status; }

/**
 * @brief Initialize I2C peripheral in master mode
 * @param[in] bus I2C bus identifier (I2C1, I2C2, I2C3)
 * @param[in] config Pointer to configuration structure
 * @return Status of initialization
 * @retval HAL_I2C_OK Initialization successful
 * @retval HAL_I2C_ERR_REINIT Bus already initialized
 * @retval HAL_I2C_ERR_BUS Unsupported mode (slave not implemented)
 *
 * @details
 * Configures the specified I2C bus with:
 * - Clock speed (standard/fast mode)
 * - APB1 clock prescaler
 * - Rise time calculation
 * - Peripheral enable
 *
 * @note Automatically enables the required RCC clock for the I2C peripheral
 */
hal_i2c_status_t hal_i2c_init(hal_i2c_bus_t bus, hal_i2c_config_t *config) {
  if (__i2c_init_status & (1 << bus))
    return HAL_I2C_ERR_REINIT; // avoid reintialization

  I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  if (config->own_address == I2C_MASTER) {
    RCC->APB1ENR |= I2C_APB1ENR_MASK(bus); // Enable base clock

    // SW Reset I2C
    I2C->CR1 &= ~I2C_CR1_PE_MASK;
    I2C->CR1 |= I2C_CR1_SWRST_MASK;
    I2C->CR1 &= ~I2C_CR1_SWRST_MASK;

    // Set bus frequency
    uint32_t bus_clock_freq = hal_clock_get_apb1clk();

    I2C->CR2 &= ~(I2C_CR2_FREQ_MASK);
    I2C->CR2 |= ((bus_clock_freq / 1000000U) & I2C_CR2_FREQ_MASK);

    // set scl freq to 100kHz or 400 kHz
    uint32_t fscl = config->clock_speed == STANDARD_MODE ? 100000U : 400000U;

    I2C->CCR &= ~((I2C_CCR_CCR_MASK) | (I2C_CCR_FS_MASK));
    if (config->clock_speed == STANDARD_MODE) {
      I2C->CCR |= bus_clock_freq / (2 * fscl);

      I2C->TRISE = (I2C->CR2 & 0x1F) + 1;
    } else if (config->clock_speed == FAST_MODE) {
      I2C->CCR |= I2C_CCR_FS_MASK | bus_clock_freq / (3 * fscl);
      I2C->TRISE =
          ((uint32_t)((I2C->CR2 & I2C_CR2_FREQ_MASK) * 300) / 1000) + 1;
    }
    I2C->CR1 |= I2C_CR1_PE_MASK;
    switch (bus) {
    case I2C1:
      __i2c_init_status += 1;
      break;
    case I2C2:
      __i2c_init_status += 2;
      break;
    case I2C3:
      __i2c_init_status += 4;
      break;
    default:
      break;
    }
    return HAL_I2C_OK;

  } else {
    // [TODO] Implement slave mode
    return HAL_I2C_ERR_BUS;
  }
}

/**
 * @brief Wait for a flag to be set with timeout
 * @param[in] reg Pointer to status register
 * @param[in] mask Flag mask to wait for
 * @return 1 if flag set, 0 if timeout occurred
 * @note Private helper function
 */
static int _wait_flag(volatile uint32_t *reg, uint32_t mask) {
  int timeout = TIMEOUT;
  while (((*reg & mask) == 0) && --timeout) {
  }
  return (timeout > 0);
}

/**
 * @brief Generate I2C START condition
 * @param[in] bus I2C bus identifier
 * @return Status of operation
 * @note Private helper function
 */
static hal_i2c_status_t _i2c_start(hal_i2c_bus_t bus) {
  I2C_GET_BASE(bus)->CR1 |= I2C_CR1_START_MASK;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_SB_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  else
    return HAL_I2C_OK;
}

/**
 * @brief Generate I2C STOP condition
 * @param[in] bus I2C bus identifier
 * @note Private helper function
 */
static void _i2c_stop(hal_i2c_bus_t bus) {
  I2C_GET_BASE(bus)->CR1 |= I2C_CR1_STOP_MASK;
}

/**
 * @brief Write device address to I2C bus
 * @param[in] bus I2C bus identifier
 * @param[in] addr 7-bit device address
 * @return Status of operation
 * @note Private helper function
 */
static hal_i2c_status_t _i2c_write_addr(hal_i2c_bus_t bus, uint8_t addr) {
  I2C_GET_BASE(bus)->DR = addr;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_ADDR_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  // clear if raeding
  (void)I2C_GET_BASE(bus)->SR1;
  (void)I2C_GET_BASE(bus)->SR2;
  return HAL_I2C_OK;
}

/**
 * @brief Write data byte to I2C bus
 * @param[in] bus I2C bus identifier
 * @param[in] data Byte to transmit
 * @return Status of operation
 * @note Private helper function
 */
static hal_i2c_status_t _i2c_write_data(hal_i2c_bus_t bus, uint8_t data) {
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_TXE_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  I2C_GET_BASE(bus)->DR = data;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_BTF_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  else
    return HAL_I2C_OK;
}

/**
 * @brief Write data to I2C device
 * @param[in] bus I2C bus identifier
 * @param[in] dev_addr 7-bit device address
 * @param[in] data Pointer to transmit buffer
 * @param[in] len Number of bytes to transmit
 * @return Status of operation
 * @retval HAL_I2C_OK Transmission successful
 * @retval HAL_I2C_ERR_TIMEOUT Bus timeout occurred
 *
 * @details
 * Performs complete I2C write transaction:
 * 1. START condition
 * 2. Address + Write bit
 * 3. Data bytes
 * 4. STOP condition
 */
hal_i2c_status_t hal_i2c_write(uint8_t bus, uint8_t dev_addr,
                               const uint8_t *data, uint16_t len) {
  hal_i2c_status_t status;

  // Generate START condition
  status = _i2c_start(bus);
  if (status != HAL_I2C_OK)
    return status;

  // Send device address with write bit (last bit 0)
  status = _i2c_write_addr(bus, dev_addr << 1);
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }

  // Send all data bytes
  for (uint16_t i = 0; i < len; i++) {
    status = _i2c_write_data(bus, data[i]);
    if (status != HAL_I2C_OK) {
      _i2c_stop(bus);
      return status;
    }
  }

  // Generate STOP condition
  _i2c_stop(bus);

  return HAL_I2C_OK;
}

/**
 * @brief Read data from I2C device
 * @param[in] bus I2C bus identifier
 * @param[in] dev_addr 7-bit device address
 * @param[out] data Pointer to receive buffer
 * @param[in] len Number of bytes to receive
 * @return Status of operation
 * @retval HAL_I2C_OK Reception successful
 * @retval HAL_I2C_ERR_TIMEOUT Bus timeout occurred
 * @retval HAL_I2C_ERR_BUS Invalid parameters
 *
 * @details
 * Performs complete I2C read transaction:
 * 1. START condition
 * 2. Address + Read bit
 * 3. Data bytes with proper ACK/NACK
 * 4. STOP condition
 */
hal_i2c_status_t hal_i2c_read(uint8_t bus, uint8_t dev_addr, uint8_t *data,
                              uint16_t len) {
  I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  if (len == 0 || data == NULL)
    return HAL_I2C_ERR_BUS;

  // Generate start condition
  if (_i2c_start(bus) != HAL_I2C_OK)
    return HAL_I2C_ERR_TIMEOUT;

  // Send device address with read bit (1)
  if (_i2c_write_addr(bus, (dev_addr << 1) | 1) != HAL_I2C_OK) {
    _i2c_stop(bus);
    return HAL_I2C_ERR_TIMEOUT;
  }

  for (uint16_t i = 0; i < len; i++) {
    if (i == len - 1) {
      // For last byte: clear ACK, set STOP, read data
      I2C->CR1 &= ~I2C_CR1_ACK_MASK; // Disable ACK
      _i2c_stop(bus);                // Generate STOP condition
    } else {
      // Enable ACK for other bytes
      I2C->CR1 |= I2C_CR1_ACK_MASK;
    }

    // Wait until RXNE (data received)
    if (!_wait_flag(&(I2C->SR1), I2C_SR1_RXNE_MASK)) {
      _i2c_stop(bus);
      return HAL_I2C_ERR_TIMEOUT;
    }

    // Read data from DR
    data[i] = (uint8_t)(I2C->DR & 0xFF);
  }

  // Re-enable ACK for future receptions
  I2C->CR1 |= I2C_CR1_ACK_MASK;

  return HAL_I2C_OK;
}

/**
 * @brief Combined write-read operation
 * @param[in] bus I2C bus identifier
 * @param[in] dev_addr 7-bit device address
 * @param[in] tx_data Pointer to transmit buffer
 * @param[in] tx_len Number of bytes to transmit
 * @param[out] rx_data Pointer to receive buffer
 * @param[in] rx_len Number of bytes to receive
 * @return Status of operation
 * @retval HAL_I2C_OK Operation successful
 * @retval HAL_I2C_ERR_TIMEOUT Bus timeout occurred
 * @retval HAL_I2C_ERR_BUS Invalid parameters
 *
 * @details
 * Performs combined I2C transaction:
 * 1. Write phase (registers/commands)
 * 2. Repeated START
 * 3. Read phase (data)
 * Handles all ACK/NACK and STOP conditions automatically
 */
hal_i2c_status_t hal_i2c_write_read(uint8_t bus, uint8_t dev_addr,
                                    const uint8_t *tx_data, uint16_t tx_len,
                                    uint8_t *rx_data, uint16_t rx_len) {
  hal_i2c_status_t status;
  I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  if (rx_len == 0 || rx_data == NULL)
    return HAL_I2C_ERR_BUS;

  // --- Write phase ---
  status = _i2c_start(bus);
  if (status != HAL_I2C_OK)
    return status;

  status = _i2c_write_addr(bus, (dev_addr << 1) | 0); // Write
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }

  for (uint16_t i = 0; i < tx_len; i++) {
    status = _i2c_write_data(bus, tx_data[i]);
    if (status != HAL_I2C_OK) {
      _i2c_stop(bus);
      return status;
    }
  }

  // --- Read phase ---
  status = _i2c_start(bus);
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }
  status = _i2c_write_addr(bus, (dev_addr << 1) | 1); // Write
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }

  I2C->CR1 |= I2C_CR1_ACK_MASK;
  // Case 1: Single byte read
  if (rx_len == 1) {
    I2C->CR1 &= ~I2C_CR1_ACK_MASK; // Disable ACK
    (void)I2C->SR1;                // Clear ADDR
    (void)I2C->SR2;
    I2C->CR1 |= I2C_CR1_STOP_MASK; // Generate STOP
    if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
      return HAL_I2C_ERR_TIMEOUT;
    rx_data[0] = I2C->DR;
    return HAL_I2C_OK;
  }
  // Case 3: More than 1 byte (generalized from bmp180_read_u16)
  else {
    (void)I2C->SR1; // Clear ADDR
    (void)I2C->SR2;

    I2C->CR1 |= I2C_CR1_ACK_MASK; // ACK first byte

    for (uint16_t i = 0; i < rx_len; i++) {
      if (i == rx_len - 2) {
        // Wait for second-to-last byte ready
        if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
          return HAL_I2C_ERR_TIMEOUT;

        rx_data[i++] = I2C->DR;

        // Disable ACK so last byte gets NACKed
        I2C->CR1 &= ~I2C_CR1_ACK_MASK;

        // Generate STOP before reading last two bytes
        I2C->CR1 |= I2C_CR1_STOP_MASK;
        if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
          return HAL_I2C_ERR_TIMEOUT;

        // Read N-1
        rx_data[i] = I2C->DR;

        break;
      }

      // Wait for RXNE for intermediate bytes
      if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
        return HAL_I2C_ERR_TIMEOUT;
      rx_data[i] = I2C->DR;
    }

    // Re-enable ACK for next transaction
    I2C->CR1 |= I2C_CR1_ACK_MASK;
    return HAL_I2C_OK;
  }
}