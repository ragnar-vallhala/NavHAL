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
 * @file i2c_f7.c
 * @brief Standardized HAL I²C master driver for STM32F7 (Cortex-M7).
 *
 * @details
 * The F7 I²C is the modern ST IP (TIMINGR / ISR-ICR / CR2-framed transfers /
 * split RXDR-TXDR), unrelated to the F4 legacy block, so this is a full
 * rewrite rather than a header swap. Master mode, blocking, with bounded
 * spin-waits so a missing/again-NACKing device returns an error instead of
 * hanging. The vendor CMakeLists selects this file in place of `i2c.c` when
 * `CONFIG_FAMILY_STM32F7` is set.
 *
 * @note TIMINGR is preset for a 16 MHz I²C clock (the reset HSI / APB1 default
 *       on the Nucleo-F767ZI). At other I²C clocks the value must be recomputed
 *       (see family/i2c_reg.h).
 */

#include "navhal_port_i2c.h"

#include "internal/hal_i2c_ops.h"
#include "navhal_port_gpio.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "family/i2c_reg.h"
#include "common/hal_i2c.h"

#define I2C_SPIN 100000U /* bounded wait iterations */

static uint8_t __i2c_init_status = 0;

static uint8_t stm32f7_i2c_get_init_status(void) { return __i2c_init_status; }

static void _cfg_pin(hal_gpio_pin_t pin) {
  hal_gpio_enable_clock(pin);
  hal_gpio_set_mode(pin, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_UP);
  hal_gpio_set_alternate_function(pin, GPIO_FUNC_I2C); /* AF4 */
  hal_gpio_set_output_type(pin, HAL_GPIO_OTYPE_OPEN_DRAIN);
  hal_gpio_set_output_speed(pin, HAL_GPIO_SPEED_HIGH);
}

static void _configure_i2c_gpio(hal_i2c_bus_t bus) {
  switch (bus) {
  case HAL_I2C_1: /* PB8 SCL / PB9 SDA */
    _cfg_pin(GPIO_PB08);
    _cfg_pin(GPIO_PB09);
    break;
  case HAL_I2C_2: /* PB10 SCL / PB11 SDA */
    _cfg_pin(GPIO_PB10);
    _cfg_pin(GPIO_PB11);
    break;
  case HAL_I2C_3: /* PA8 SCL / PC9 SDA */
    _cfg_pin(GPIO_PA08);
    _cfg_pin(GPIO_PC09);
    break;
  default:
    break;
  }
}

/** @brief Spin until @p flag sets in ISR; abort on NACK / timeout. */
static hal_status_t _wait_isr(volatile I2C_Reg_Typedef *I2C, uint32_t flag) {
  uint32_t spin = I2C_SPIN;
  while (spin--) {
    uint32_t isr = I2C->ISR;
    if (isr & flag)
      return HAL_OK;
    if (isr & I2C_ISR_NACKF) {
      I2C->ICR = I2C_ICR_NACKCF;
      return HAL_ERR_IO;
    }
  }
  return HAL_ERR_TIMEOUT;
}

static hal_status_t stm32f7_i2c_init(hal_i2c_bus_t bus,
                                     const hal_i2c_config_t *config) {
  if (config == NULL)
    return HAL_ERR_INVALID_ARG;
  if (__i2c_init_status & (1 << bus))
    return HAL_ERR_NOT_INITIALIZED; /* avoid re-init (matches F4 contract) */
  if (config->own_address != I2C_MASTER)
    return HAL_ERR_IO; /* slave mode unimplemented */

  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  RCC->APB1ENR |= I2C_APB1ENR_MASK(bus);
  _configure_i2c_gpio(bus);

  I2C->CR1 &= ~I2C_CR1_PE; /* PE=0 required to program TIMINGR */
  I2C->TIMINGR = (config->clock_speed == HAL_I2C_SPEED_FAST)
                     ? I2C_TIMINGR_FM_16MHZ
                     : I2C_TIMINGR_SM_16MHZ;
  I2C->CR1 |= I2C_CR1_PE;

  __i2c_init_status |= (1 << bus);
  return HAL_OK;
}

static hal_status_t stm32f7_i2c_write(hal_i2c_bus_t bus, uint8_t dev_addr,
                           const uint8_t *data, uint16_t len) {
  if (data == NULL || len == 0)
    return HAL_ERR_IO;
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  I2C->CR2 = I2C_CR2_SADD7(dev_addr) | I2C_CR2_NBYTES(len) | I2C_CR2_AUTOEND |
             I2C_CR2_START;

  for (uint16_t i = 0; i < len; i++) {
    hal_status_t s = _wait_isr(I2C, I2C_ISR_TXIS);
    if (s != HAL_OK)
      return s;
    I2C->TXDR = data[i];
  }
  hal_status_t s = _wait_isr(I2C, I2C_ISR_STOPF);
  I2C->ICR = I2C_ICR_STOPCF;
  return s;
}

static hal_status_t stm32f7_i2c_read(hal_i2c_bus_t bus, uint8_t dev_addr,
                                     uint8_t *data,
                          uint16_t len) {
  if (data == NULL || len == 0)
    return HAL_ERR_IO;
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  I2C->CR2 = I2C_CR2_SADD7(dev_addr) | I2C_CR2_NBYTES(len) | I2C_CR2_RD_WRN |
             I2C_CR2_AUTOEND | I2C_CR2_START;

  for (uint16_t i = 0; i < len; i++) {
    hal_status_t s = _wait_isr(I2C, I2C_ISR_RXNE);
    if (s != HAL_OK)
      return s;
    data[i] = (uint8_t)I2C->RXDR;
  }
  hal_status_t s = _wait_isr(I2C, I2C_ISR_STOPF);
  I2C->ICR = I2C_ICR_STOPCF;
  return s;
}

static hal_status_t stm32f7_i2c_write_read(hal_i2c_bus_t bus, uint8_t dev_addr,
                                const uint8_t *tx_data, uint16_t tx_len,
                                uint8_t *rx_data, uint16_t rx_len) {
  if (tx_data == NULL || rx_data == NULL || tx_len == 0 || rx_len == 0)
    return HAL_ERR_IO;
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);

  /* Write phase: SOFTEND (no AUTOEND) so a repeated START can follow. */
  I2C->CR2 = I2C_CR2_SADD7(dev_addr) | I2C_CR2_NBYTES(tx_len) | I2C_CR2_START;
  for (uint16_t i = 0; i < tx_len; i++) {
    hal_status_t s = _wait_isr(I2C, I2C_ISR_TXIS);
    if (s != HAL_OK)
      return s;
    I2C->TXDR = tx_data[i];
  }
  hal_status_t s = _wait_isr(I2C, I2C_ISR_TC);
  if (s != HAL_OK)
    return s;

  /* Read phase: repeated START with AUTOEND. */
  I2C->CR2 = I2C_CR2_SADD7(dev_addr) | I2C_CR2_NBYTES(rx_len) | I2C_CR2_RD_WRN |
             I2C_CR2_AUTOEND | I2C_CR2_START;
  for (uint16_t i = 0; i < rx_len; i++) {
    s = _wait_isr(I2C, I2C_ISR_RXNE);
    if (s != HAL_OK)
      return s;
    rx_data[i] = (uint8_t)I2C->RXDR;
  }
  s = _wait_isr(I2C, I2C_ISR_STOPF);
  I2C->ICR = I2C_ICR_STOPCF;
  return s;
}

/*===========================================================================
 * DMA-backed I2C register read (STM32F7).
 *
 * Write the register pointer with the same CR2 framing as hal_i2c_write_read,
 * then read N bytes via DMA: enable CR1.RXDMAEN, point a P2M stream at RXDR, and
 * start a repeated-START read with AUTOEND so the peripheral issues STOP after
 * the last byte. On DMA transfer-complete the stream IRQ clears RXDMAEN and
 * fires the caller's callback.
 *===========================================================================*/
#if NAVHAL_CONFIG_DRV_I2C_DMA
#include "navhal_port_dma.h"
#include "navhal_port_interrupt.h"

static void (*_i2c_dma_rx_callback)(void) = NULL;
static hal_dma_config_t _active_i2c_dma_config;
static hal_i2c_bus_t _active_i2c_dma_bus;
static void _i2c_dma_irq_handler(void);

/** Map a DMA1 stream index to its NVIC line (F7 I2C DMA requests are on DMA1). */
static hal_irq_t _dma1_stream_irq(uint8_t s) {
  switch (s) {
  case 0: return DMA1_Stream0_IRQn;
  case 1: return DMA1_Stream1_IRQn;
  case 2: return DMA1_Stream2_IRQn;
  case 3: return DMA1_Stream3_IRQn;
  case 4: return DMA1_Stream4_IRQn;
  case 5: return DMA1_Stream5_IRQn;
  case 6: return DMA1_Stream6_IRQn;
  default: return DMA1_Stream7_IRQn;
  }
}

hal_status_t hal_i2c_read_regs_dma(hal_i2c_bus_t bus, uint8_t dev_addr,
                                   uint8_t reg, const hal_dma_config_t *dma_cfg,
                                   void (*callback)(void)) {
  if (dma_cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);
  if (!I2C)
    return HAL_ERR_NOT_INITIALIZED;

  /* The DMA writes into the caller's buffer; reject one no DMA can reach. The
   * matching invalidate happens on completion in _i2c_dma_irq_handler. */
  hal_status_t gs = navhal_dma_rx_guard((const void *)(uintptr_t)dma_cfg->dst_addr);
  if (gs != HAL_OK)
    return gs;

  /* Write phase: send the register pointer (SOFTEND so a repeated START can
   * follow) — identical framing to hal_i2c_write_read's write phase. */
  I2C->CR2 = I2C_CR2_SADD7(dev_addr) | I2C_CR2_NBYTES(1) | I2C_CR2_START;
  hal_status_t s = _wait_isr(I2C, I2C_ISR_TXIS);
  if (s != HAL_OK)
    return s;
  I2C->TXDR = reg;
  s = _wait_isr(I2C, I2C_ISR_TC);
  if (s != HAL_OK)
    return s;

  /* Read phase via DMA. The driver owns the peripheral-side config (RXDR
   * source, P2M, 8-bit); the caller supplies controller/stream/channel, the
   * destination buffer, and the byte count. */
  _i2c_dma_rx_callback = callback;
  _active_i2c_dma_bus = bus;
  _active_i2c_dma_config = *dma_cfg;
  _active_i2c_dma_config.direction = HAL_DMA_DIR_P2M;
  _active_i2c_dma_config.src_addr = (uint32_t)(uintptr_t)&I2C->RXDR;
  _active_i2c_dma_config.src_inc = 0;
  _active_i2c_dma_config.dst_inc = 1;
  _active_i2c_dma_config.data_width = HAL_DMA_DATA_WIDTH_8;

  I2C->CR1 |= I2C_CR1_RXDMAEN;

  hal_dma_init(&_active_i2c_dma_config);
  if (_active_i2c_dma_config.controller == HAL_DMA_CONTROLLER_1) {
    hal_irq_t irq = _dma1_stream_irq(_active_i2c_dma_config.stream);
    hal_interrupt_attach_callback(irq, _i2c_dma_irq_handler);
    hal_interrupt_enable_with_priority(irq, HAL_IRQ_PRIORITY_DEFAULT);
  }
  hal_dma_start(&_active_i2c_dma_config);

  /* Repeated START read with AUTOEND: the peripheral STOPs after NBYTES. */
  I2C->CR2 = I2C_CR2_SADD7(dev_addr) |
             I2C_CR2_NBYTES((uint8_t)_active_i2c_dma_config.data_count) |
             I2C_CR2_RD_WRN | I2C_CR2_AUTOEND | I2C_CR2_START;
  return HAL_OK;
}

static void _i2c_dma_irq_handler(void) {
  if (!hal_dma_transfer_complete(&_active_i2c_dma_config))
    return;
  hal_dma_clear_flags(&_active_i2c_dma_config);
  /* AUTOEND has already issued STOP; just drop the DMA request enable. */
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(_active_i2c_dma_bus);
  if (I2C)
    I2C->CR1 &= ~I2C_CR1_RXDMAEN;
  /* Drop the CPU's stale cached copy of the buffer the DMA just filled, before
   * the callback (and the caller) read it. 8-bit transfers: items == bytes. */
  navhal_dma_rx_finish((void *)(uintptr_t)_active_i2c_dma_config.dst_addr,
                       _active_i2c_dma_config.data_count);
  if (_i2c_dma_rx_callback)
    _i2c_dma_rx_callback();
}

#endif /* NAVHAL_CONFIG_DRV_I2C_DMA */

/** @brief The F7 I2C backend. The DMA entry point above stays a public
 *  capability-gated symbol; it is not part of the portable contract. */
/* The F7 has no CR1.SWRST: clearing PE is itself the reset, per RM0410 --
 * the peripheral drops its state machine and most of CR2/ISR when disabled.
 * Same observable contract as the F4 deinit, different mechanism. */
static hal_status_t stm32f7_i2c_deinit(hal_i2c_bus_t bus) {
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(bus);
  if (!I2C)
    return HAL_ERR_INVALID_ARG;

  I2C->CR1 &= ~I2C_CR1_PE;

  __i2c_init_status &= (uint8_t)~(1u << bus);
  return HAL_OK;
}

const hal_i2c_ops_t _hal_i2c_ops = {
    .init = stm32f7_i2c_init,
    .deinit = stm32f7_i2c_deinit,
    .write = stm32f7_i2c_write,
    .read = stm32f7_i2c_read,
    .write_read = stm32f7_i2c_write_read,
    .get_init_status = stm32f7_i2c_get_init_status,
};
