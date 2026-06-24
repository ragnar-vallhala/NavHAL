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

hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg) {
  if (cfg == NULL || _get_usart(uart) == NULL)
    return HAL_ERR_INVALID_ARG;
  _uart_hw_init(uart, cfg->baudrate);
  return HAL_OK;
}

hal_status_t hal_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
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

/* -------------------------------------------------------------------------- *
 * IDLE-line interrupt → user callback.
 *
 * The IDLE line fires at the inter-frame gap of a byte-oriented protocol
 * (e.g. SBUS/iBUS), letting a consumer block until a whole frame has arrived
 * instead of polling. Pairs naturally with circular DMA RX: the DMA fills the
 * ring, and IDLE signals "a burst just ended — go drain it".
 * -------------------------------------------------------------------------- */

/** UART -> index into the per-UART tables (UART1=0, UART2=1, UART6=2). */
static inline int _uart_idx(hal_uart_t uart) {
  return (uart == HAL_UART_1) ? 0 : (uart == HAL_UART_6) ? 2 : 1;
}

static void (*_uart_idle_cb[3])(void) = {0};

/* Demux + clear: on the shared USART IRQ, run only if the IDLE flag is set.
 * F4 has no IDLE-clear bit — the flag is cleared by a read of SR followed by a
 * read of DR. At IDLE time the line is quiet and DMA has already drained DR, so
 * the dummy DR read steals no in-flight byte. */
static void _uart_idle_handle(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return;
  if (usart->SR & USART_SR_IDLE) {
    (void)usart->DR; /* SR was just read above; DR read clears IDLE */
    void (*cb)(void) = _uart_idle_cb[_uart_idx(uart)];
    if (cb)
      cb();
  }
}

/* The interrupt registry takes a void(void) callback, so one thin trampoline
 * per USART line carries the UART identity. */
static void _uart_idle_irq_usart1(void) { _uart_idle_handle(HAL_UART_1); }
static void _uart_idle_irq_usart2(void) { _uart_idle_handle(HAL_UART_2); }
static void _uart_idle_irq_usart6(void) { _uart_idle_handle(HAL_UART_6); }

hal_status_t hal_uart_attach_idle_callback(hal_uart_t uart,
                                           void (*callback)(void)) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart || !callback)
    return HAL_ERR_INVALID_ARG;

  hal_irq_t irq;
  void (*trampoline)(void);
  if (uart == HAL_UART_1) {
    irq = USART1_IRQn;
    trampoline = _uart_idle_irq_usart1;
  } else if (uart == HAL_UART_6) {
    irq = USART6_IRQn;
    trampoline = _uart_idle_irq_usart6;
  } else {
    irq = USART2_IRQn;
    trampoline = _uart_idle_irq_usart2;
  }

  _uart_idle_cb[_uart_idx(uart)] = callback;
  hal_interrupt_attach_callback(irq, trampoline);

  (void)usart->SR; /* clear any stale IDLE (SR then DR) before arming */
  (void)usart->DR;
  usart->CR1 |= USART_CR1_IDLEIE;

  /* Maskable (BASEPRI-managed) priority so the callback may call an RTOS
   * *_from_isr primitive — same contract as the DMA-completion ISRs. */
  hal_interrupt_enable_with_priority(irq, HAL_IRQ_PRIORITY_DEFAULT);
  return HAL_OK;
}

hal_status_t hal_uart_detach_idle_callback(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return HAL_ERR_INVALID_ARG;
  usart->CR1 &= ~USART_CR1_IDLEIE;
  _uart_idle_cb[_uart_idx(uart)] = NULL;
  return HAL_OK;
}

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return HAL_ERR_INVALID_ARG;

  // NOTE: this is a RAW byte primitive — no '\n'->"\r\n" translation. The old
  // CRLF injection corrupted any BINARY stream containing a 0x0A byte (e.g.
  // vayu's framed telemetry over a blocking-write UART), inserting a stray
  // 0x0D and breaking framing/CRC. Text callers that want CRLF must emit it.
  while (!(usart->SR & USART_SR_TXE))
    ;
  usart->DR = c;
  return HAL_OK;
}

/** @brief Unified helper: convert a number to decimal text and transmit it. */
static void _uart_write_number(hal_uart_t uart, uint32_t num, int is_signed) {
  char buf[12];
  int i = 0;

  if (is_signed && (int32_t)num < 0) {
    hal_uart_write_char(uart, '-');
    num = (uint32_t)(-(int32_t)num);
  }

  if (num == 0) {
    hal_uart_write_char(uart, '0');
    return;
  }

  while (num > 0) {
    buf[i++] = (char)('0' + (num % 10));
    num /= 10;
  }

  while (i--) {
    hal_uart_write_char(uart, buf[i]);
  }
}

hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num) {
  _uart_write_number(uart, (uint32_t)num, 1);
  return HAL_OK;
}

hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num) {
  _uart_write_number(uart, num, 0);
  return HAL_OK;
}

hal_status_t hal_uart_write_float(hal_uart_t uart, float num) {
  if (num < 0) {
    hal_uart_write_char(uart, '-');
    num = -num;
  }
  uint32_t integer = (uint32_t)num;
  _uart_write_number(uart, integer, 0);
  hal_uart_write_char(uart, '.');
  float fractional = num - (float)integer;
  // 5 decimal places with rounding
  _uart_write_number(uart, (uint32_t)(fractional * 100000.0f + 0.5f), 0);
  return HAL_OK;
}

hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s) {
  if (!s)
    return HAL_ERR_INVALID_ARG;
  while (*s) {
    hal_uart_write_char(uart, *s++);
  }
  return HAL_OK;
}

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length) {
  if (!data)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < length; i++) {
    hal_uart_write_char(uart, (char)data[i]);
  }
  return HAL_OK;
}

char hal_uart_read_char(hal_uart_t uart) {
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

bool hal_uart_available(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  return (usart && (usart->SR & USART_SR_RXNE));
}

uint32_t hal_uart_read_until(hal_uart_t uart, char *buffer, uint32_t maxlen,
                             char delimiter) {
  uint32_t i = 0;
  if (!buffer || maxlen == 0)
    return 0;

  while (i < maxlen - 1) {
    while (!hal_uart_available(uart))
      ;
    char c = hal_uart_read_char(uart);
    if (c == delimiter)
      break;
    buffer[i++] = c;
  }
  buffer[i] = '\0';
  return i;
}

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
      /* USART6_TX uses the Stream7/Ch5 alternate mapping, NOT Stream6/Ch5: DMA2
       * Stream6 is owned by the SDIO write-DMA (vendor/stm32/sdio/sdio.c), so
       * sharing Stream6 makes the SDIO IRQ steal USART6_TX completions and wedges
       * telemetry (e.g. dead after the first calibration/log SD write). Stream7 is
       * conflict-free. Consumers attach their TX-complete callback to
       * DMA2_Stream7_IRQn (see vayu src/comm/channel.c). */
      p.stream = 7;
      p.channel = 5;
      p.irq = DMA2_Stream7_IRQn;
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

/** Configured circular RX-buffer length per UART (UART1=0, UART2=1, UART6=2);
 *  0 until hal_uart_init_dma_rx() runs. Used to turn NDTR into a write index. */
static uint16_t _uart_rx_dma_len[3] = {0};

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
  _uart_rx_dma_len[_uart_idx(uart)] = length;
  return HAL_OK;
}

hal_status_t hal_uart_dma_rx_index(hal_uart_t uart, uint16_t *out_index) {
  if (!out_index)
    return HAL_ERR_INVALID_ARG;
  _uart_dma_params_t p = _get_uart_dma_params(uart, 0);
  uint16_t len = _uart_rx_dma_len[_uart_idx(uart)];
  if (!p.controller || len == 0)
    return HAL_ERR_INVALID_ARG; /* RX DMA not configured for this UART */

  /* NDTR counts down from `len` to 0 as the DMA fills the circular buffer, so
   * the next-write index is len - NDTR. Reading the correct stream's NDTR is
   * the whole point — callers must not guess the stream themselves. */
  uint16_t ndtr = (uint16_t)(p.controller->STREAM[p.stream].NDTR & 0xFFFFu);
  *out_index = (uint16_t)(len - ndtr);
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
