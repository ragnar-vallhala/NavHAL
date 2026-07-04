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
 * @file uart_f7.c
 * @brief Standardized HAL UART driver for STM32F7 (Cortex-M7) — USART1/2/3/6.
 *
 * @details
 * Implements the same `hal_uart_*` contract as the F4 `uart.c`, but against the
 * STM32F7 USART IP (RM0410 §34): status is read from a read-only `ISR`, error
 * flags are cleared through `ICR`, and data uses split `RDR`/`TDR` registers
 * instead of the F4's single `DR`. The vendor CMakeLists selects this file in
 * place of `uart.c` when `CONFIG_FAMILY_STM32F7` is set, so the F4 reference
 * driver is left untouched.
 *
 * The numeric/string formatting helpers below are intentionally duplicated from
 * `uart.c` — they are register-agnostic and will be de-duplicated when the UART
 * driver moves to the vendor-backend vtable (roadmap M9).
 *
 * @note Default frame configuration: 8 data bits, no parity, 1 stop bit.
 * @note Blocking transfers are polling-mode; the DMA-backed API
 *       (hal_uart_write_dma / init_dma_rx) is at the bottom, gated by
 *       NAVHAL_CONFIG_DRV_UART_DMA. DMA buffers are coherent while the L1
 *       D-cache stays off (the current bring-up default); once it is enabled
 *       they will need clean/invalidate or DTCM placement (NAVHAL_DTCM_NOINIT).
 */

#include "navhal_port_uart.h"
#include "navhal_port_clock.h"
#include "navhal_port_gpio.h"
#include "navhal_port_interrupt.h"
#include "family/rcc_reg.h"
#include "family/uart_reg.h"
#include <stdint.h>
#if NAVHAL_CONFIG_DRV_UART_DMA
#include "navhal_port_dma.h"
#endif

static inline volatile UARTx_Reg_Typedef *_get_usart(hal_uart_t uart) {
  return (volatile UARTx_Reg_Typedef *)GET_USARTx_BASE(uart);
}

/** @brief USART2/3 are on APB1; USART1/6 are on APB2. */
static inline uint32_t _uart_periph_clk(hal_uart_t uart) {
  return (uart == HAL_UART_2 || uart == HAL_UART_3) ? hal_clock_get_apb1clk()
                                                    : hal_clock_get_apb2clk();
}

/** @brief Enable the peripheral clock for the specified UART. */
static void _enable_uart_clock(hal_uart_t uart) {
  if (uart == HAL_UART_1)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  else if (uart == HAL_UART_2)
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  else if (uart == HAL_UART_3)
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
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
  } else if (uart == HAL_UART_3) {
    /* Nucleo-F767ZI ST-LINK virtual COM port: PD8 TX / PD9 RX, AF7. */
    hal_gpio_set_alternate_function(GPIO_PD08, HAL_GPIO_AF7); // TX
    hal_gpio_set_alternate_function(GPIO_PD09, HAL_GPIO_AF7); // RX
  } else if (uart == HAL_UART_6) {
    hal_gpio_set_alternate_function(GPIO_PC06, HAL_GPIO_AF8); // TX
    hal_gpio_set_alternate_function(GPIO_PC07, HAL_GPIO_AF8); // RX
  }
}

/** @brief Core hardware initialization for a UART. */
static void _uart_hw_init(hal_uart_t uart, uint32_t baudrate) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart || baudrate == 0)
    return;

  _enable_uart_clock(uart);
  _configure_uart_gpio(uart);

  /* BRR can only be written while the USART is disabled (UE=0); it is at reset
   * here. With default oversampling-by-16, USARTDIV == BRR == fck / baud. */
  usart->CR1 = 0;
  uint32_t clk = _uart_periph_clk(uart);
  usart->BRR = (clk + (baudrate / 2)) / baudrate; // rounded

  /* Enable the peripheral and the transmitter/receiver. */
  usart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
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
    usart->CR1 |= USART_CR1_RXNEIE;
  else
    usart->CR1 &= ~USART_CR1_RXNEIE;

  if (tx_en)
    usart->CR1 |= USART_CR1_TXEIE;
  else
    usart->CR1 &= ~USART_CR1_TXEIE;

  hal_irq_t irq = (uart == HAL_UART_1)   ? USART1_IRQn
                  : (uart == HAL_UART_3) ? USART3_IRQn
                  : (uart == HAL_UART_6) ? USART6_IRQn
                                         : USART2_IRQn;
  hal_interrupt_enable(irq);
  return HAL_OK;
}

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return HAL_ERR_INVALID_ARG;

  /* RAW byte primitive — no '\n'->"\r\n" translation (matches uart.c). */
  while (!(usart->ISR & USART_ISR_TXE))
    ;
  usart->TDR = (uint32_t)(uint8_t)c;
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

  /* Clear sticky error flags via ICR (F7 does not auto-clear on data read). */
  uint32_t status = usart->ISR;
  if (status & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE)) {
    usart->ICR = USART_ICR_ORECF | USART_ICR_NCF | USART_ICR_FECF |
                 USART_ICR_PECF;
    (void)usart->RDR;
    return 0;
  }

  while (!(usart->ISR & USART_ISR_RXNE))
    ;
  return (char)(usart->RDR & 0xFFU);
}

bool hal_uart_available(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  return (usart && (usart->ISR & USART_ISR_RXNE));
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
 * DMA-backed UART transmit/receive (STM32F7).
 *
 * Same shape as the F4 backend in uart.c, adapted to the F7 USART IP: the DMA
 * peripheral address is the split TDR (base+0x28) on TX and RDR (base+0x24) on
 * RX, not the F4's single DR (base+0x04). USART1/2/6 keep the F4 DMA request
 * map; USART3 (the Nucleo-F767ZI console) is the F7-specific addition.
 *===========================================================================*/
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA

typedef struct {
  DMA_Typedef *controller;
  uint8_t stream;
  uint8_t channel;
  uint8_t irq;
  uint32_t periph_addr;
} _uart_dma_params_t;

/** @brief Resolve DMA controller/stream/channel/IRQ and the TDR|RDR address for
 *  a UART and direction (RM0410 Tables 27/28). */
static _uart_dma_params_t _get_uart_dma_params(hal_uart_t uart, int is_tx) {
  _uart_dma_params_t p = {0};
  uint32_t base = 0;
  if (uart == HAL_UART_1) {
    p.controller = DMA2;
    base = USART1_BASE;
    if (is_tx) { p.stream = 7; p.channel = 4; p.irq = DMA2_Stream7_IRQn; }
    else       { p.stream = 2; p.channel = 4; p.irq = DMA2_Stream2_IRQn; }
  } else if (uart == HAL_UART_2) {
    p.controller = DMA1;
    base = USART2_BASE;
    if (is_tx) { p.stream = 6; p.channel = 4; p.irq = DMA1_Stream6_IRQn; }
    else       { p.stream = 5; p.channel = 4; p.irq = DMA1_Stream5_IRQn; }
  } else if (uart == HAL_UART_3) {
    p.controller = DMA1;
    base = USART3_BASE;
    if (is_tx) { p.stream = 3; p.channel = 4; p.irq = DMA1_Stream3_IRQn; }
    else       { p.stream = 1; p.channel = 4; p.irq = DMA1_Stream1_IRQn; }
  } else if (uart == HAL_UART_6) {
    p.controller = DMA2;
    base = USART6_BASE;
    if (is_tx) { p.stream = 6; p.channel = 5; p.irq = DMA2_Stream6_IRQn; }
    else       { p.stream = 1; p.channel = 5; p.irq = DMA2_Stream1_IRQn; }
  }
  if (base)
    p.periph_addr = base + (is_tx ? 0x28u : 0x24u); /* TDR : RDR */
  return p;
}

/* Re-arm cache: index = USARTn*2 + (RX?1:0), n in {1:0, 2:1, 3:2, 6:3}. */
static uint8_t _uart_dma_initialized[8] = {0};
static int _uart_dma_idx(hal_uart_t uart, int is_tx) {
  int n = (uart == HAL_UART_1)   ? 0
          : (uart == HAL_UART_2) ? 1
          : (uart == HAL_UART_3) ? 2
                                 : 3;
  return n * 2 + (is_tx ? 0 : 1);
}

hal_status_t hal_uart_write_dma(hal_uart_t uart, const uint8_t *data,
                                uint16_t length) {
  if (!data || length == 0)
    return HAL_ERR_INVALID_ARG;

  _uart_dma_params_t p = _get_uart_dma_params(uart, 1);
  if (!p.controller)
    return HAL_ERR_INVALID_ARG;

  /* Flush the caller's buffer so the DMA transmits the CPU's latest writes
   * (no-op for DTCM/uncached buffers and cache-off builds; rejects ITCM). */
  hal_status_t cs = navhal_dma_tx_prepare(data, length);
  if (cs != HAL_OK)
    return cs;

  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  usart->CR3 |= USART_CR3_DMAT;

  int idx = _uart_dma_idx(uart, 1);
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
    /* Re-arm: wait for the previous transfer, then repoint memory + count. */
    DMA_Stream_Typedef *s = &p.controller->STREAM[p.stream];
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

/*
 * NOTE (D-cache): this is a *circular* RX DMA the CPU reads live, so the driver
 * cannot invalidate on the caller's behalf. On a cache-on build place @p buffer
 * in DTCM (::NAVHAL_DTCM) — DMA1/DMA2 reach it and it is never cached, so it
 * stays coherent for free — or invalidate the region yourself before each read.
 * The guard below only rejects a DMA-unreachable (ITCM) buffer.
 */
hal_status_t hal_uart_init_dma_rx(hal_uart_t uart, uint8_t *buffer,
                                  uint16_t length) {
  if (!buffer || length == 0)
    return HAL_ERR_INVALID_ARG;

  hal_status_t gs = navhal_dma_rx_guard(buffer);
  if (gs != HAL_OK)
    return gs;

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
  _uart_dma_initialized[_uart_dma_idx(uart, 0)] = 1;
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

#endif /* NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA */
