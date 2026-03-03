#define CORTEX_M4
#include "navhal.h"

int main() {
  systick_init(1000); /**< Initialize SysTick for 1ms ticks */
  uart2_init(9600);

  /* --- DMA benchmark --- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  int n = hal_get_tick();
  int iter = 100;
  while (iter--)
    uart2_write_string_dma("Hello World\n\r"); /**< DMA transfer */
  uart2_write_string("DMA done: ");
  uart2_write(hal_get_tick() - n);
  uart2_write_string(" ticks\n\r");
#else
  /* --- Polling benchmark (fallback) --- */
  int n = hal_get_tick();
  int iter = 100;
  while (iter--)
    uart2_write_string("Hello World\n\r"); /**< Polling transfer */
  uart2_write_string("Poll done: ");
  uart2_write(hal_get_tick() - n);
  uart2_write_string(" ticks\n\r");
#endif
  return 0;
}