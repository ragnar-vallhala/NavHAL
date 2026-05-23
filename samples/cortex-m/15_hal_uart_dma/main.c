#define CORTEX_M4
#include "navhal.h"

int main() {
  hal_timebase_init(1000); /**< Initialize SysTick for 1ms ticks */
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate=9600});

  /* --- DMA benchmark --- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  int n = hal_timebase_get_tick();
  int iter = 100;
  while (iter--)
    hal_uart_write_string_dma(HAL_UART_6, "Hello World\n\r"); /**< DMA transfer */
  hal_uart_write_string(HAL_UART_6, "DMA done: ");
  hal_uart_print(HAL_UART_6, hal_timebase_get_tick() - n);
  hal_uart_write_string(HAL_UART_6, " ticks\n\r");
#else
  /* --- Polling benchmark (fallback) --- */
  int n = hal_timebase_get_tick();
  int iter = 100;
  while (iter--)
    hal_uart_write_string(HAL_UART_2, "Hello World\n\r"); /**< Polling transfer */
  hal_uart_write_string(HAL_UART_2, "Poll done: ");
  hal_uart_print(HAL_UART_2, hal_timebase_get_tick() - n);
  hal_uart_write_string(HAL_UART_2, " ticks\n\r");
#endif
  return 0;
}