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
 * @note All blocking transfers are polling-mode. DMA backend is not yet wired
 *       for F7 (needs Cortex-M7 cache-coherency handling — port plan F7-5).
 */

#include "navhal_port_uart.h"
#include "navhal_port_clock.h"
#include "navhal_port_gpio.h"
#include "navhal_port_interrupt.h"
#include "family/rcc_reg.h"
#include "family/uart_reg.h"
#include <stdint.h>

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
