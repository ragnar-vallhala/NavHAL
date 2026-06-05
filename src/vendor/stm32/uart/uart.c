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
 * @file uart.c
 * @brief Standardized HAL UART driver for STM32F4 (Cortex-M4) — USART1/2/6.
 *
 * @details
 * Implements the standardized `hal_uart_*` API declared in
 * `port/cortex-m4/navhal_port_uart.h`: initialization, blocking character/number/string/
 * buffer transmission, blocking reception, interrupt enable, and an optional
 * DMA transmit/receive backend.
 *
 * @note Default frame configuration: 8 data bits, no parity, 1 stop bit.
 * @note All blocking transfers are polling-mode.
 */

#include "internal/hal_uart_ops.h"
#include "navhal_port_uart.h"
#include "navhal_port_clock.h"
#include "navhal_port_gpio.h"
#include "navhal_port_interrupt.h"
#include "family/rcc_reg.h"
#include "family/uart_reg.h"
#include <stdint.h>
#ifdef _UART_BACKEND_DMA
#include "navhal_port_dma.h"
#endif

static inline volatile UARTx_Reg_Typedef *_get_usart(hal_uart_t uart) {
  return (volatile UARTx_Reg_Typedef *)GET_USARTx_BASE(uart);
}

/** @brief Enable the peripheral clock for the specified UART. */
static void _enable_uart_clock(hal_uart_t uart) {
  if (uart == HAL_UART_1)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  else if (uart == HAL_UART_2)
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  else if (uart == HAL_UART_6)
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
}

/** @brief Configure the GPIO alternate-function pins for the specified UART. */
static void _configure_uart_gpio(hal_uart_t uart) {
  if (uart == HAL_UART_1) {
    hal_gpio_set_alternate_function(GPIO_PB06, HAL_GPIO_AF7); // TX
    hal_gpio_set_alternate_function(GPIO_PB07, HAL_GPIO_AF7); // RX
  } else if (uart == HAL_UART_2) {
    hal_gpio_set_alternate_function(GPIO_PA02, HAL_GPIO_AF7); // TX
    hal_gpio_set_alternate_function(GPIO_PA03, HAL_GPIO_AF7); // RX
  } else if (uart == HAL_UART_6) {
    hal_gpio_set_alternate_function(GPIO_PC06, HAL_GPIO_AF8); // TX
    hal_gpio_set_alternate_function(GPIO_PC07, HAL_GPIO_AF8); // RX
  }
}

/** @brief Core hardware initialization for a UART. */
static void _uart_hw_init(hal_uart_t uart, uint32_t baudrate) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return;

  _enable_uart_clock(uart);
  _configure_uart_gpio(uart);

  uint32_t clk =
      (uart == HAL_UART_2) ? hal_clock_get_apb1clk() : hal_clock_get_apb2clk();
  usart->BRR = (clk + (baudrate / 2)) / baudrate; // Rounded BRR

  // Clear flags
  volatile uint32_t tmp = usart->SR;
  tmp = usart->DR;
  (void)tmp;

  usart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static hal_status_t stm32_uart_init(hal_uart_t uart,
                                    const hal_uart_config_t *cfg) {
  /* cfg non-NULL: validated by the public layer. */
  if (_get_usart(uart) == NULL)
    return HAL_ERR_INVALID_ARG;
  _uart_hw_init(uart, cfg->baudrate);
  return HAL_OK;
}

static hal_status_t stm32_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                                uint8_t tx_en) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return HAL_ERR_INVALID_ARG;

  if (rx_en)
    usart->CR1 |= UART_CR1_RXNEIE;
  else
    usart->CR1 &= ~UART_CR1_RXNEIE;

  if (tx_en)
    usart->CR1 |= (1 << 7); // TXEIE is bit 7 in CR1
  else
    usart->CR1 &= ~(1 << 7);

  // Also enable in NVIC
  hal_irq_t irq = (uart == HAL_UART_1)   ? USART1_IRQn
                  : (uart == HAL_UART_6) ? USART6_IRQn
                                         : USART2_IRQn;
  hal_interrupt_enable(irq);
  return HAL_OK;
}

static hal_status_t stm32_uart_write_char(hal_uart_t uart, char c) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return HAL_ERR_INVALID_ARG;

  if (c == '\n') {
    while (!(usart->SR & USART_SR_TXE))
      ;
    usart->DR = '\r';
  }

  while (!(usart->SR & USART_SR_TXE))
    ;
  usart->DR = c;
  return HAL_OK;
}

static char stm32_uart_read_char(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return 0;

  // Clear errors if any
  uint32_t status = usart->SR;
  if (status & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
    (void)usart->DR;
    return 0;
  }

  while (!(usart->SR & USART_SR_RXNE))
    ;
  return (char)usart->DR;
}

static bool stm32_uart_available(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  return (usart && (usart->SR & USART_SR_RXNE));
}

const hal_uart_ops_t _hal_uart_ops = {
    .init = stm32_uart_init,
    .enable_interrupt = stm32_uart_enable_interrupt,
    .write_char = stm32_uart_write_char,
    .read_char = stm32_uart_read_char,
    .available = stm32_uart_available,
};

/*===========================================================================
 * DMA-backed UART transmit/receive — compiled only when the DMA backend is on.
 *===========================================================================*/
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

typedef struct {
  DMA_Typedef *controller;
  uint8_t stream;
  uint8_t channel;
  uint8_t irq;
  uint32_t periph_addr;
} _uart_dma_params_t;

/** @brief Resolve DMA parameters for a given UART and direction. */
static _uart_dma_params_t _get_uart_dma_params(hal_uart_t uart, int is_tx) {
  _uart_dma_params_t p = {0};
  if (uart == HAL_UART_1) {
    p.controller = DMA2;
    p.periph_addr = USART1_BASE + 0x04;
    if (is_tx) {
      p.stream = 7;
      p.channel = 4;
      p.irq = DMA2_Stream7_IRQn;
    } else {
      p.stream = 2;
      p.channel = 4;
      p.irq = DMA2_Stream2_IRQn;
    }
  } else if (uart == HAL_UART_2) {
    p.controller = DMA1;
    p.periph_addr = USART2_BASE + 0x04;
    if (is_tx) {
      p.stream = 6;
      p.channel = 4;
      p.irq = DMA1_Stream6_IRQn;
    } else {
      p.stream = 5;
      p.channel = 4;
      p.irq = DMA1_Stream5_IRQn;
    }
  } else if (uart == HAL_UART_6) {
    p.controller = DMA2;
    p.periph_addr = USART6_BASE + 0x04;
    if (is_tx) {
      p.stream = 6;
      p.channel = 5;
      p.irq = DMA2_Stream6_IRQn;
    } else {
      p.stream = 1;
      p.channel = 5;
      p.irq = DMA2_Stream1_IRQn;
    }
  }
  return p;
}

/**
 * @brief Tracks initialisation state for DMA streams to avoid redundant setups.
 *
 * Indices: UART1_TX=0, UART1_RX=1, UART2_TX=2, UART2_RX=3, UART6_TX=4,
 * UART6_RX=5.
 */
static uint8_t _uart_dma_initialized[6] = {0};

hal_status_t hal_uart_write_dma(hal_uart_t uart, const uint8_t *data,
                                uint16_t length) {
  if (!data || length == 0)
    return HAL_ERR_INVALID_ARG;

  _uart_dma_params_t p = _get_uart_dma_params(uart, 1);
  if (!p.controller)
    return HAL_ERR_INVALID_ARG;

  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  usart->CR3 |= USART_CR3_DMAT;

  int idx = (uart == HAL_UART_1) ? 0 : (uart == HAL_UART_2) ? 2 : 4;
  hal_dma_config_t cfg = {
      .controller =
          (p.controller == DMA1) ? HAL_DMA_CONTROLLER_1 : HAL_DMA_CONTROLLER_2,
      .stream = p.stream,
      .channel = p.channel,
      .direction = HAL_DMA_DIR_M2P,
      .src_addr = (uint32_t)data,
      .dst_addr = p.periph_addr,
      .data_count = length,
      .src_inc = 1,
      .dst_inc = 0,
      .data_width = HAL_DMA_DATA_WIDTH_8,
      .priority = HAL_DMA_PRIORITY_HIGH,
  };

  if (!_uart_dma_initialized[idx]) {
    hal_dma_init(&cfg);
    _uart_dma_initialized[idx] = 1;
  } else {
    DMA_Stream_Typedef *s = &p.controller->STREAM[p.stream];
    /* Safe-Async: Wait for previous transfer before starting new one */
    while (s->CR & DMA_SxCR_EN)
      ;
    s->M0AR = (uint32_t)data;
    s->NDTR = length;
  }

  hal_dma_clear_flags(&cfg);
  hal_interrupt_enable((hal_irq_t)p.irq);
  hal_dma_start(&cfg);
  return HAL_OK;
}

hal_status_t hal_uart_init_dma_rx(hal_uart_t uart, uint8_t *buffer,
                                  uint16_t length) {
  if (!buffer || length == 0)
    return HAL_ERR_INVALID_ARG;

  _uart_dma_params_t p = _get_uart_dma_params(uart, 0);
  if (!p.controller)
    return HAL_ERR_INVALID_ARG;

  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  usart->CR3 |= USART_CR3_DMAR;

  hal_dma_config_t cfg = {
      .controller =
          (p.controller == DMA1) ? HAL_DMA_CONTROLLER_1 : HAL_DMA_CONTROLLER_2,
      .stream = p.stream,
      .channel = p.channel,
      .direction = HAL_DMA_DIR_P2M,
      .src_addr = p.periph_addr,
      .dst_addr = (uint32_t)buffer,
      .data_count = length,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = HAL_DMA_DATA_WIDTH_8,
      .priority = HAL_DMA_PRIORITY_MEDIUM,
      .circular = 1,
  };

  hal_dma_init(&cfg);
  hal_dma_start(&cfg);

  int idx = (uart == HAL_UART_1) ? 1 : (uart == HAL_UART_2) ? 3 : 5;
  _uart_dma_initialized[idx] = 1;
  return HAL_OK;
}

hal_status_t hal_uart_write_string_dma(hal_uart_t uart, const char *s) {
  if (!s)
    return HAL_ERR_INVALID_ARG;
  uint16_t len = 0;
  while (s[len])
    len++;
  return hal_uart_write_dma(uart, (const uint8_t *)s, len);
}

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */
