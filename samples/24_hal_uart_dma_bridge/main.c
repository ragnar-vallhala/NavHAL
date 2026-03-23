#define CORTEX_M4
#include "core/cortex-m4/config.h"
#include "core/cortex-m4/dma_reg.h"
#include "navhal.h"

#define BUF_SIZE 256

uint8_t u2_rx_buf[BUF_SIZE];
uint8_t u6_rx_buf[BUF_SIZE];

uint16_t u2_head = 0;
uint16_t u6_head = 0;

int main(void) {
  systick_init(1000);

  /* Initialize UART2 and UART6 at 115200 bps */
  uart2_init(115200);
  uart6_init(115200);

#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  /* Start DMA circular reception for both UARTs */
  uart2_init_dma_rx(u2_rx_buf, BUF_SIZE);
  uart6_init_dma_rx(u6_rx_buf, BUF_SIZE);

  /* Optional start message */
  uart2_write_string_dma("Bridge Started: UART2 <-> UART6\r\n");
  uart6_write_string_dma("Bridge Started: UART6 <-> UART2\r\n");

  while (1) {
    /* Check UART2 RX buffer via DMA NDTR */
    uint16_t u2_tail = BUF_SIZE - DMA1->STREAM[5].NDTR;
    if (u2_tail != u2_head) {
      if (u2_tail > u2_head) {
        uart6_write_dma(&u2_rx_buf[u2_head], u2_tail - u2_head);
      } else {
        uart6_write_dma(&u2_rx_buf[u2_head], BUF_SIZE - u2_head);
        if (u2_tail > 0) {
          uart6_write_dma(&u2_rx_buf[0], u2_tail);
        }
      }
      u2_head = u2_tail;
    }

    /* Check UART6 RX buffer via DMA NDTR */
    uint16_t u6_tail = BUF_SIZE - DMA2->STREAM[1].NDTR;
    if (u6_tail != u6_head) {
      if (u6_tail > u6_head) {
        uart2_write_dma(&u6_rx_buf[u6_head], u6_tail - u6_head);
      } else {
        uart2_write_dma(&u6_rx_buf[u6_head], BUF_SIZE - u6_head);
        if (u6_tail > 0) {
          uart2_write_dma(&u6_rx_buf[0], u6_tail);
        }
      }
      u6_head = u6_tail;
    }
  }
#else
  /* Fallback if DMA is not enabled */
  while (1) {
    if (uart2_available()) {
      uart6_write_char(uart2_read_char());
    }
    if (uart6_available()) {
      uart2_write_char(uart6_read_char());
    }
  }
#endif

  return 0;
}
