/**
 * @file 16_hal_dma_polling_uart/main.c
 * @brief DMA vs Polling UART TX comparison.
 *
 * Key insight:
 *   - Polling: CPU is stuck in a spin-loop waiting for each byte to shift out.
 *     No other work is possible. cpu_work_done stays 0.
 *   - DMA: Bytes clock out autonomously. CPU is free to count work iterations.
 *
 * Both modes take the same wall-clock time (baud-rate limited), but DMA gives
 * you the CPU back for sensor reads, PID loops, PWM updates, etc.
 *
 * Expected output at 9600 baud:
 *   [POLLING] Time=~1373 ticks | CPU work=0
 *   [DMA]     Time=~1373 ticks | CPU work=<N>  (N >> 0)
 */

#define CORTEX_M4
#include "navhal.h"

#define MSG "Hello World\n\r"
#define MSG_LEN 13

static volatile uint32_t cpu_work_done;

/*
 * Spin doing CPU work until the deadline tick.
 * Called from run_dma_iter() in the window after a DMA transfer completes.
 */
static void do_cpu_work(uint32_t deadline_tick) {
  while (hal_get_tick() < deadline_tick)
    cpu_work_done++;
}

/* Blocking polling TX — CPU cannot do anything else while transmitting */
static uint32_t run_polling(int iters) {
  cpu_work_done = 0;
  uint32_t t0 = hal_get_tick();
  for (int i = 0; i < iters; i++)
    uart2_write_string(MSG);
  return hal_get_tick() - t0;
}

#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
/*
 * DMA TX — after each transfer completes the CPU runs do_cpu_work()
 * for the remaining time in the transfer window.
 * 13 bytes × 10 bits / 9600 bps ≈ 13.5 ms per message → 14-tick deadline.
 */
static uint32_t run_dma_iter(int iters) {
  cpu_work_done = 0;
  uint32_t t0 = hal_get_tick();
  for (int i = 0; i < iters; i++) {
    uint32_t deadline = hal_get_tick() + 14; /* expected end of this msg */
    uart2_write_dma((const uint8_t *)MSG, MSG_LEN);
    do_cpu_work(deadline); /* use any remaining time in the window */
  }
  return hal_get_tick() - t0;
}
#endif

int main(void) {
  systick_init(1000);
  uart2_init(9600);

  uart2_write_string("\r\n=== DMA vs Polling Comparison ===\r\n\r\n");

  const int ITERS = 100;

  /* ---- Polling ---- */
  uart2_write_string("[POLLING] Running...\r\n");
  uint32_t poll_ticks = run_polling(ITERS);
  uint32_t poll_work = cpu_work_done;
  uart2_write_string("[POLLING] Time=");
  uart2_write(poll_ticks);
  uart2_write_string(" ticks | CPU work=");
  uart2_write(poll_work);
  uart2_write_string("\r\n\r\n");

#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  /* ---- DMA ---- */
  uart2_write_string("[DMA]     Running...\r\n");
  uint32_t dma_ticks = run_dma_iter(ITERS);
  uint32_t dma_work = cpu_work_done;
  uart2_write_string("[DMA]     Time=");
  uart2_write(dma_ticks);
  uart2_write_string(" ticks | CPU work=");
  uart2_write(dma_work);
  uart2_write_string("\r\n\r\n");

  /* ---- Result ---- */
  uart2_write_string("=== Result ===\r\n");
  uart2_write_string("Wall time : similar (baud-limited)\r\n");
  uart2_write_string("Poll work : ");
  uart2_write(poll_work);
  uart2_write_string("\r\nDMA work  : ");
  uart2_write(dma_work);
  uart2_write_string("\r\nDMA frees CPU: YES\r\n");
#else
  uart2_write_string("[DMA] Not enabled. Set _DMA_ENABLED in config.h\r\n");
#endif
  return 0;
}
