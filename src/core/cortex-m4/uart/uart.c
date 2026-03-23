/**
 * @file uart.c
 * @brief UART initialization and I/O functions for STM32F4 UART1, UART2, and
 * UART6.
 *
 * This file provides the implementation of UART communication functions
 * for STM32F401RE microcontroller. It supports USART1, USART2, and USART6
 * peripherals with the following features:
 * - Baud rate configuration
 * - Character, integer, float, and string transmission
 * - Character reception
 * - Data availability checking
 * - Blocking (polling) implementations
 *
 * The implementation uses GPIO alternate functions for UART pins and
 * provides both generic (UART instance parameter) and specific functions.
 *
 * @note All functions are blocking and will wait for hardware operations to
 * complete
 * @note Default configuration: 8 data bits, no parity, 1 stop bit
 *
 * @ingroup HAL_UART
 * @author Ashutosh Vishwakarma
 * @date 2025-07-23
 */

#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/uart_reg.h"
#ifdef _UART_BACKEND_DMA
#include "core/cortex-m4/dma.h"
#include "core/cortex-m4/interrupt.h"
#endif

static inline volatile UARTx_Reg_Typedef *_get_usart(hal_uart_t uart) {
  return (volatile UARTx_Reg_Typedef *)GET_USARTx_BASE(uart);
}

/**
 * @brief Enable the peripheral clock for the specified UART
 */
static void _enable_uart_clock(hal_uart_t uart) {
  if (uart == UART1)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  else if (uart == UART2)
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  else if (uart == UART6)
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
}

/**
 * @brief Configure the GPIO pins for the specified UART
 */
static void _configure_uart_gpio(hal_uart_t uart) {
  if (uart == UART1) {
    hal_gpio_set_alternate_function(GPIO_PB06, GPIO_AF07); // TX
    hal_gpio_set_alternate_function(GPIO_PB07, GPIO_AF07); // RX
  } else if (uart == UART2) {
    hal_gpio_set_alternate_function(GPIO_PA02, GPIO_AF07); // TX
    hal_gpio_set_alternate_function(GPIO_PA03, GPIO_AF07); // RX
  } else if (uart == UART6) {
    hal_gpio_set_alternate_function(GPIO_PC06, GPIO_AF08); // TX
    hal_gpio_set_alternate_function(GPIO_PC07, GPIO_AF08); // RX
  }
}

/**
 * @brief Core hardware initialization for UART
 */
static void _uart_hw_init(hal_uart_t uart, uint32_t baudrate) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return;

  _enable_uart_clock(uart);
  _configure_uart_gpio(uart);

  uint32_t clk =
      (uart == UART2) ? hal_clock_get_apb1clk() : hal_clock_get_apb2clk();
  usart->BRR = (clk + (baudrate / 2)) / baudrate; // Rounded BRR

  // Clear flags
  volatile uint32_t tmp = usart->SR;
  tmp = usart->DR;
  (void)tmp;

  usart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/**
 * @brief Initialize the specified UART peripheral
 */
void uart_init(uint32_t baudrate, hal_uart_t uart) {
  _uart_hw_init(uart, baudrate);
}

void uart1_init(uint32_t baudrate) { _uart_hw_init(UART1, baudrate); }
void uart2_init(uint32_t baudrate) {
  _uart_hw_init(UART2, baudrate);
  // UART2 is used for standard debug, optionally enable interrupt
  _get_usart(UART2)->CR1 |= UART_CR1_RXNEIE;
}
void uart6_init(uint32_t baudrate) { _uart_hw_init(UART6, baudrate); }

/**
 * @brief Transmit a single character via the specified UART
 */
void uart_write_char(char c, hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  if (!usart)
    return;

  if (c == '\n') {
    while (!(usart->SR & USART_SR_TXE))
      ;
    usart->DR = '\r';
  }

  while (!(usart->SR & USART_SR_TXE))
    ;
  usart->DR = c;
}

/**
 * @brief Unified helper for number to string conversion and transmission
 */
static void _uart_write_number(uint32_t num, int is_signed, hal_uart_t uart) {
  char buf[12];
  int i = 0;

  if (is_signed && (int32_t)num < 0) {
    uart_write_char('-', uart);
    num = (uint32_t)(-(int32_t)num);
  }

  if (num == 0) {
    uart_write_char('0', uart);
    return;
  }

  while (num > 0) {
    buf[i++] = (char)('0' + (num % 10));
    num /= 10;
  }

  while (i--) {
    uart_write_char(buf[i], uart);
  }
}

void uart_write_int(int32_t num, hal_uart_t uart) {
  _uart_write_number((uint32_t)num, 1, uart);
}

void uart_write_uint(uint32_t num, hal_uart_t uart) {
  _uart_write_number(num, 0, uart);
}

void uart_write_float(float num, hal_uart_t uart) {
  if (num < 0) {
    uart_write_char('-', uart);
    num = -num;
  }
  uint32_t integer = (uint32_t)num;
  _uart_write_number(integer, 0, uart);
  uart_write_char('.', uart);
  float fractional = num - (float)integer;
  // 5 decimal places with rounding
  _uart_write_number((uint32_t)(fractional * 100000.0f + 0.5f), 0, uart);
}

void uart_write_string(const char *s, hal_uart_t uart) {
  if (!s)
    return;
  while (*s) {
    uart_write_char(*s++, uart);
  }
}

void uart_write_buf(const uint8_t *data, uint16_t length, hal_uart_t uart) {
  if (!data)
    return;
  for (uint16_t i = 0; i < length; i++) {
    uart_write_char((char)data[i], uart);
  }
}

char uart_read_char(hal_uart_t uart) {
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

int uart_available(hal_uart_t uart) {
  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  return (usart && (usart->SR & USART_SR_RXNE));
}

uint32_t uart_read_until(char *buffer, uint32_t maxlen, char delimiter,
                         hal_uart_t uart) {
  uint32_t i = 0;
  if (!buffer || maxlen == 0)
    return 0;

  while (i < maxlen - 1) {
    while (!uart_available(uart))
      ;
    char c = uart_read_char(uart);
    if (c == delimiter)
      break;
    buffer[i++] = c;
  }
  buffer[i] = '\0';
  return i;
}

/*===========================================================================
 * DMA-backed UART transmit — compiled only when _UART_BACKEND_DMA is defined
 *===========================================================================*/
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

/**
 * @brief DMA configuration for USART2 TX.
 *
 * USART2_TX is on DMA1 Stream6, Channel 4 (per STM32F4 datasheet).
 * The peripheral address points to USART2->DR.
 * Caller must fill in src_addr and data_count before calling dma_start().
 */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

typedef struct {
  DMA_Typedef *controller;
  uint8_t stream;
  uint8_t channel;
  uint8_t irq;
  uint32_t periph_addr;
} _uart_dma_params_t;

/**
 * @brief Helper to get DMA parameters for a given UART and direction
 */
static _uart_dma_params_t _get_uart_dma_params(hal_uart_t uart, int is_tx) {
  _uart_dma_params_t p = {0};
  if (uart == UART1) {
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
  } else if (uart == UART2) {
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
  } else if (uart == UART6) {
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
 * UART6_RX=5
 */
static uint8_t _uart_dma_initialized[6] = {0};

void uart_write_dma(const uint8_t *data, uint16_t length, hal_uart_t uart) {
  if (!data || length == 0)
    return;

  _uart_dma_params_t p = _get_uart_dma_params(uart, 1);
  if (!p.controller)
    return;

  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  usart->CR3 |= USART_CR3_DMAT;

  int idx = (uart == UART1) ? 0 : (uart == UART2) ? 2 : 4;
  dma_config_t cfg = {
      .controller =
          (p.controller == DMA1) ? DMA_CONTROLLER_1 : DMA_CONTROLLER_2,
      .stream = p.stream,
      .channel = p.channel,
      .direction = DMA_DIR_M2P,
      .src_addr = (uint32_t)data,
      .dst_addr = p.periph_addr,
      .data_count = length,
      .src_inc = 1,
      .dst_inc = 0,
      .data_width = DMA_DATA_WIDTH_8,
      .priority = DMA_PRIORITY_HIGH,
  };

  if (!_uart_dma_initialized[idx]) {
    dma_init(&cfg);
    _uart_dma_initialized[idx] = 1;
  } else {
    DMA_Stream_Typedef *s = &p.controller->STREAM[p.stream];
    /* Safe-Async: Wait for previous transfer before starting new one */
    while (s->CR & DMA_SxCR_EN)
      ;
  }

  dma_clear_flags(&cfg);
  hal_enable_interrupt((IRQn_Type)p.irq);
  dma_start(&cfg);
}

void uart_init_dma_rx(uint8_t *buffer, uint16_t length, hal_uart_t uart) {
  if (!buffer || length == 0)
    return;

  _uart_dma_params_t p = _get_uart_dma_params(uart, 0);
  if (!p.controller)
    return;

  volatile UARTx_Reg_Typedef *usart = _get_usart(uart);
  usart->CR3 |= USART_CR3_DMAR;

  dma_config_t cfg = {
      .controller =
          (p.controller == DMA1) ? DMA_CONTROLLER_1 : DMA_CONTROLLER_2,
      .stream = p.stream,
      .channel = p.channel,
      .direction = DMA_DIR_P2M,
      .src_addr = p.periph_addr,
      .dst_addr = (uint32_t)buffer,
      .data_count = length,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = DMA_DATA_WIDTH_8,
      .priority = DMA_PRIORITY_MEDIUM,
      .circular = 1,
  };

  dma_init(&cfg);
  dma_start(&cfg);

  int idx = (uart == UART1) ? 1 : (uart == UART2) ? 3 : 5;
  _uart_dma_initialized[idx] = 1;
}

void uart_write_string_dma(const char *s, hal_uart_t uart) {
  if (!s)
    return;
  uint16_t len = 0;
  while (s[len])
    len++;
  uart_write_dma((const uint8_t *)s, len, uart);
}

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */
