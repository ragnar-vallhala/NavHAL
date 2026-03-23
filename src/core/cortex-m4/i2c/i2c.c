// i2c.c
#include "core/cortex-m4/i2c.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/i2c_reg.h"
#include "core/cortex-m4/rcc_reg.h"
#include <stdbool.h>
#include <stdint.h>

// BUS  | SCL   | SDA
// I²C1 → PB6 / PB7
// I²C2 → PB10 / PB11
// I²C3 → PA8 / PB4

#define TIMEOUT 1000000
#define I2C1_EN 1
#define I2C2_EN 2
#define I2C3_EN 4
static uint8_t __i2c_init_status = 0;

// Initializes the I2C peripheral

int _wait_flag(volatile uint32_t *reg, uint32_t mask);

uint8_t hal_i2c_get_init_status(void) { return __i2c_init_status; }

#ifdef _DMA_ENABLED
#include "core/cortex-m4/interrupt.h"

static void (*_i2c_dma_rx_callback)(void) = NULL;
static dma_config_t _active_i2c_dma_config;
static void _i2c_dma_irq_handler(void);

hal_i2c_status_t hal_i2c_read_regs_dma(uint8_t bus, uint8_t dev_addr,
                                       uint8_t reg, const dma_config_t *dma_cfg,
                                       void (*callback)(void)) {
  I2C_Reg_Typedef *I2Cx =
      I2C_GET_BASE(bus); // Changed get_i2c_base to I2C_GET_BASE
  if (!I2Cx)
    return HAL_I2C_ERR_REINIT;

  int timeout = TIMEOUT;
  while ((I2Cx->SR2 & I2C_SR2_BUSY) && --timeout) {
  }
  if (timeout == 0)
    return HAL_I2C_ERR_BUS;

  /* START -> DEV_ADDR(W) -> REG_ADDR */
  I2Cx->CR1 |= I2C_CR1_START_MASK;
  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_SB_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  I2Cx->DR = (dev_addr << 1) & ~0x01; // Write
  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_ADDR_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  (void)I2Cx->SR1;
  (void)I2Cx->SR2; // Clear ADDR

  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_TXE_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  I2Cx->DR = reg;
  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_BTF_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  /* RESTART -> DEV_ADDR(R) */
  I2Cx->CR1 |= I2C_CR1_START_MASK;
  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_SB_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  I2Cx->DR = (dev_addr << 1) | 0x01; // Read
  if (!_wait_flag(&(I2Cx->SR1), I2C_SR1_ADDR_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  /* Now switch to DMA for the remaining RX transaction */
  _i2c_dma_rx_callback = callback;

  // Make a local copy to know what flags to clear during interrupt
  _active_i2c_dma_config = *dma_cfg;

  // Set ACK to ensure we pull data normally until DMA flags LAST
  I2Cx->CR1 |= I2C_CR1_ACK_MASK;
  // I2C DMA specific setup: Enable DMAEN in CR2 + Enable LAST for terminating
  // NACK
  I2Cx->CR2 |= I2C_CR2_LAST; // LAST bit
  I2Cx->CR2 |= I2C_CR2_DMAEN;

  // Init/Start the DMA (CR, NDTR, M0AR config + Enable)
  dma_init(&_active_i2c_dma_config);

  // Register our internal handler for the chosen stream
  if (_active_i2c_dma_config.controller == DMA_CONTROLLER_1) {
    if (_active_i2c_dma_config.stream == 0) {
      hal_interrupt_attach_callback(DMA1_Stream0_IRQn, _i2c_dma_irq_handler);
      hal_enable_interrupt(DMA1_Stream0_IRQn);
    } else if (_active_i2c_dma_config.stream == 5) {
      hal_interrupt_attach_callback(DMA1_Stream5_IRQn, _i2c_dma_irq_handler);
      hal_enable_interrupt(DMA1_Stream5_IRQn);
    }
  }

  dma_start(&_active_i2c_dma_config);

  (void)I2Cx->SR1;
  (void)I2Cx->SR2; // Clear ADDR to release clock stretching and let DMA read

  return HAL_I2C_OK;
}

// Internal callback for I2C DMA RX completion
static void _i2c_dma_irq_handler(void) {
  if (dma_transfer_complete(&_active_i2c_dma_config)) {
    dma_clear_flags(&_active_i2c_dma_config);

    // Stop and Disable I2C DMA gracefully
    I2C_Reg_Typedef *I2Cx = I2C_GET_BASE(I2C1);
    if (I2Cx) {
      I2Cx->CR2 &= ~I2C_CR2_DMAEN;
      I2Cx->CR2 &= ~I2C_CR2_LAST;
      I2Cx->CR1 |= I2C_CR1_ACK_MASK;
      I2Cx->CR1 |= I2C_CR1_STOP_MASK;
    }

    if (_i2c_dma_rx_callback) {
      _i2c_dma_rx_callback();
    }
  }
}

#endif
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

int _wait_flag(volatile uint32_t *reg, uint32_t mask) {
  int timeout = TIMEOUT;
  while (((*reg & mask) == 0) && --timeout) {
  }
  return (timeout > 0);
}

hal_i2c_status_t _i2c_start(hal_i2c_bus_t bus) {
  I2C_GET_BASE(bus)->CR1 |= I2C_CR1_START_MASK;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_SB_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  else
    return HAL_I2C_OK;
}

void _i2c_stop(hal_i2c_bus_t bus) {
  I2C_GET_BASE(bus)->CR1 |= I2C_CR1_STOP_MASK;
}
hal_i2c_status_t _i2c_write_addr(hal_i2c_bus_t bus, uint8_t addr) {
  I2C_GET_BASE(bus)->DR = addr;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_ADDR_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  // clear if raeding
  (void)I2C_GET_BASE(bus)->SR1;
  (void)I2C_GET_BASE(bus)->SR2;
  return HAL_I2C_OK;
}
hal_i2c_status_t _i2c_write_data(hal_i2c_bus_t bus, uint8_t data) {
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_TXE_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  I2C_GET_BASE(bus)->DR = data;
  if (!_wait_flag(&(I2C_GET_BASE(bus)->SR1), I2C_SR1_BTF_MASK))
    return HAL_I2C_ERR_TIMEOUT;
  else
    return HAL_I2C_OK;
}

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

  status = _i2c_write_addr(bus, (dev_addr << 1) | 0); // write
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

  // --- Repeated START + read address ---
  status = _i2c_start(bus); // repeated start
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }

  status = _i2c_write_addr(bus, (dev_addr << 1) | 1); // read
  if (status != HAL_I2C_OK) {
    _i2c_stop(bus);
    return status;
  }

  // Ensure ACK enabled by default for multi-byte
  I2C->CR1 |= I2C_CR1_ACK_MASK;

  /********** Case N == 1 **********/
  if (rx_len == 1) {
    // For a single byte read: disable ACK, clear ADDR, generate STOP, then read
    // DR
    I2C->CR1 &= ~I2C_CR1_ACK_MASK; // NACK the single byte
    (void)I2C->SR1;                // clear ADDR
    (void)I2C->SR2;
    I2C->CR1 |= I2C_CR1_STOP_MASK; // generate STOP
    if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
      return HAL_I2C_ERR_TIMEOUT;
    rx_data[0] = (uint8_t)I2C->DR;
    return HAL_I2C_OK;
  }

  /********** Case N == 2 **********/
  if (rx_len == 2) {
    // For two bytes: clear ACK, set POS, clear ADDR, wait for BTF, then read
    // two bytes.
    I2C->CR1 &= ~I2C_CR1_ACK_MASK; // NACK the last byte
    I2C->CR1 |= I2C_CR1_POS_MASK;  // set POS for N=2

    (void)I2C->SR1; // clear ADDR
    (void)I2C->SR2;

    // Wait until both bytes received
    if (!_wait_flag(&I2C->SR1, I2C_SR1_BTF_MASK))
      return HAL_I2C_ERR_TIMEOUT;

    I2C->CR1 |= I2C_CR1_STOP_MASK; // generate STOP

    rx_data[0] = (uint8_t)I2C->DR; // read byte N-1
    rx_data[1] = (uint8_t)I2C->DR; // read byte N

    // restore POS bit
    I2C->CR1 &= ~I2C_CR1_POS_MASK;

    return HAL_I2C_OK;
  }

  /********** Case N > 2 **********/
  // Clear ADDR first
  (void)I2C->SR1;
  (void)I2C->SR2;

  // Read N-3 bytes using RXNE
  uint16_t i = 0;
  for (; i < (rx_len - 3); ++i) {
    if (!_wait_flag(&I2C->SR1, I2C_SR1_RXNE_MASK))
      return HAL_I2C_ERR_TIMEOUT;
    rx_data[i] = (uint8_t)I2C->DR;
  }

  // Now we are at the point to handle the last three bytes
  // Wait for BTF: indicates two bytes are in DR/shift reg
  if (!_wait_flag(&I2C->SR1, I2C_SR1_BTF_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  // At this point there are at least two bytes pending. Disable ACK so the last
  // byte will be NACKed.
  I2C->CR1 &= ~I2C_CR1_ACK_MASK;

  // Read the next byte (N-2)
  rx_data[i++] = (uint8_t)I2C->DR;

  // Wait for BTF again for the remaining two bytes
  if (!_wait_flag(&I2C->SR1, I2C_SR1_BTF_MASK))
    return HAL_I2C_ERR_TIMEOUT;

  // Generate STOP before reading the last two bytes
  I2C->CR1 |= I2C_CR1_STOP_MASK;

  // Read remaining two bytes
  rx_data[i++] = (uint8_t)I2C->DR; // N-1
  rx_data[i] = (uint8_t)I2C->DR;   // N

  // Re-enable ACK for next transaction (optional, but safe)
  I2C->CR1 |= I2C_CR1_ACK_MASK;

  return HAL_I2C_OK;
}
