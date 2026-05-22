#define CORTEX_M4
#include "navhal_port_config.h"
#include "family/dma_reg.h"
#include "navhal.h"

#define BUF_SIZE 256

uint8_t u2_rx_buf[BUF_SIZE];
uint8_t u6_rx_buf[BUF_SIZE];

uint16_t u2_head = 0;
uint16_t u6_head = 0;

int main(void) {
  hal_timebase_init(1000);

  /* Initialize HAL_UART_2 and HAL_UART_6 at 115200 bps */
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate=115200});

#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  /* Start DMA circular reception for both UARTs */
  hal_uart_init_dma_rx(HAL_UART_2, u2_rx_buf, BUF_SIZE);
  hal_uart_init_dma_rx(HAL_UART_6, u6_rx_buf, BUF_SIZE);

  /* Optional start message */
  hal_uart_write_string_dma(HAL_UART_2, "Bridge Started: HAL_UART_2 <-> HAL_UART_6\r\n");
  hal_uart_write_string_dma(HAL_UART_6, "Bridge Started: HAL_UART_6 <-> HAL_UART_2\r\n");

  while (1) {
    /* Check HAL_UART_2 RX buffer via DMA NDTR */
    uint16_t u2_tail = BUF_SIZE - DMA1->STREAM[5].NDTR;
    if (u2_tail != u2_head) {
      if (u2_tail > u2_head) {
        hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[u2_head], u2_tail - u2_head);
      } else {
        hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[u2_head], BUF_SIZE - u2_head);
        if (u2_tail > 0) {
          hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[0], u2_tail);
        }
      }
      u2_head = u2_tail;
    }

    /* Check HAL_UART_6 RX buffer via DMA NDTR */
    uint16_t u6_tail = BUF_SIZE - DMA2->STREAM[1].NDTR;
    if (u6_tail != u6_head) {
      if (u6_tail > u6_head) {
        hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[u6_head], u6_tail - u6_head);
      } else {
        hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[u6_head], BUF_SIZE - u6_head);
        if (u6_tail > 0) {
          hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[0], u6_tail);
        }
      }
      u6_head = u6_tail;
    }
  }
#else
  /* Fallback if DMA is not enabled */
  while (1) {
    if (hal_uart_available(HAL_UART_2)) {
      hal_uart_write_char(HAL_UART_6, hal_uart_read_char(HAL_UART_2));
    }
    if (hal_uart_available(HAL_UART_6)) {
      hal_uart_write_char(HAL_UART_2, hal_uart_read_char(HAL_UART_6));
    }
  }
#endif

  return 0;
}
