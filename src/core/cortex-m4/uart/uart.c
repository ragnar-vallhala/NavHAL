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
static dma_config_t _uart2_dma_tx_cfg = {
    .controller = DMA_CONTROLLER_1,
    .stream = 6,
    .channel = 4,
    .direction = DMA_DIR_M2P,
    .src_addr = 0,                  /* set at runtime */
    .dst_addr = USART2_BASE + 0x04, /* &USART2->DR */
    .data_count = 0,                /* set at runtime */
    .src_inc = 1,
    .dst_inc = 0,
    .data_width = DMA_DATA_WIDTH_8,
    .priority = DMA_PRIORITY_HIGH,
    .circular = 0,
};

/**
 * @brief Transmit a buffer over USART2 using DMA (blocking until done).
 *
 * Enables USART2 CR3.DMAT so the UART hardware triggers DMA requests.
 * Waits for the DMA transfer-complete flag before returning.
 *
 * @param data   Pointer to the byte buffer to transmit.
 * @param length Number of bytes to send.
 */
/**
 * @brief Transmit a buffer over USART2 using DMA (blocking until done).
 *
 * dma_init() is called only on the FIRST invocation so we never hit its
 * blocking while(EN) spin from inside SysTick_Handler.  Subsequent calls
 * wait for the previous transfer to finish (TC flag set), then reload
 * NDTR / M0AR and re-enable the stream directly.
 *
 * @param data   Pointer to the byte buffer to transmit.
 * @param length Number of bytes to send.
 */
void uart2_write_dma(const uint8_t *data, uint16_t length) {
  if (!data || length == 0)
    return;

  volatile UARTx_Reg_Typedef *usart = GET_USARTx_BASE(2);
  if (usart == NULL)
    return;

  /* Enable UART DMA TX request once */
  usart->CR3 |= USART_CR3_DMAT;

  /* --- First-time init -------------------------------------------------- */
  static uint8_t s_dma_initialised = 0;
  if (!s_dma_initialised) {
    _uart2_dma_tx_cfg.src_addr = (uint32_t)data;
    _uart2_dma_tx_cfg.data_count = length;
    dma_init(&_uart2_dma_tx_cfg);        /* sets up CR, enables TCIE     */
    dma_clear_flags(&_uart2_dma_tx_cfg); /* clear stale flags             */
    hal_enable_interrupt(DMA1_Stream6_IRQn);
    s_dma_initialised = 1;
  } else {
    /* --- Subsequent calls: restart without blocking dma_init() ---------- */
    /* The stream must be disabled before touching NDTR/M0AR.              */
    /* We wait for TC (non-blocking guard) instead of waiting for EN=0,   */
    /* because we know the previous transfer completed (read_lock was 0).  */
    DMA_Stream_Typedef *s = &DMA1->STREAM[6];

    /* Disable stream — stream finishes quickly after TC, so this should   */
    /* clear almost instantly.  Hard-cap the spin to avoid ISR lockup.     */
    s->CR &= ~DMA_SxCR_EN;
    uint32_t guard = 0;
    while ((s->CR & DMA_SxCR_EN) && guard++ < 1000u)
      ;
    /* If the stream is still somehow active after the guard, abort this   */
    /* flush and let the next SysTick pick it up.                          */
    if (s->CR & DMA_SxCR_EN)
      return;

    dma_clear_flags(&_uart2_dma_tx_cfg);

    /* Reload transfer parameters */
    s->M0AR = (uint32_t)data;
    s->NDTR = length;

    /* Re-enable TCIE in case it was cleared, then start */
    s->CR |= DMA_SxCR_TCIE;
  }

  /* Enable DMA interrupt and start the transfer */
  hal_enable_interrupt(DMA1_Stream6_IRQn);
  dma_start(&_uart2_dma_tx_cfg);
}

/**
 * @brief Transmit a null-terminated string over USART2 using DMA.
 *
 * @note The string must remain valid until the transfer completes (no copy).
 *       For stack-allocated strings consider making a static/global buffer.
 *
 * @param s Null-terminated string to transmit.
 */
void uart2_write_string_dma(const char *s) {
  if (!s)
    return;
  uint16_t len = 0;
  while (s[len])
    len++;
  uart2_write_dma((const uint8_t *)s, len);
}

static dma_config_t _uart1_dma_rx_cfg = {
    .controller = DMA_CONTROLLER_2,
    .stream = 2,
    .channel = 4,
    .direction = DMA_DIR_P2M,
    .src_addr = USART1_BASE + 0x04, /* &USART1->DR */
    .dst_addr = 0,                  /* set at runtime */
    .data_count = 0,                /* set at runtime */
    .src_inc = 0,
    .dst_inc = 1,
    .data_width = DMA_DATA_WIDTH_8,
    .priority = DMA_PRIORITY_MEDIUM,
    .circular = 1,
};

void uart1_init_dma_rx(uint8_t *buffer, uint16_t length, uint32_t baudrate) {
  if (!buffer || length == 0)
    return;

  volatile UARTx_Reg_Typedef *usart = GET_USARTx_BASE(1);
  if (usart == NULL)
    return;

  /* Peripheral init */
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  hal_gpio_set_alternate_function(GPIO_PB06, GPIO_AF07); // TX
  hal_gpio_set_alternate_function(GPIO_PB07, GPIO_AF07); // RX

  /* Configure Baud Rate */
  usart->BRR = (hal_clock_get_apb2clk() + (baudrate / 2)) / baudrate;

  /* Setup DMA Transfer */
  _uart1_dma_rx_cfg.dst_addr = (uint32_t)buffer;
  _uart1_dma_rx_cfg.data_count = length;

  dma_init(&_uart1_dma_rx_cfg);

  /* Enable UART DMA RX request */
  usart->CR3 |= USART_CR3_DMAR;
  usart->CR1 |= USART_CR1_RE | USART_CR1_UE;

  dma_start(&_uart1_dma_rx_cfg);
}

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */
