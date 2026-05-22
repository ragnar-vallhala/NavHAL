/**
 * @file main.c
 * @brief Performance reading and writing to SDIO SD card.
 *
 * @details
 * - Initializes PLL (84MHz) and SDIO.
 * - Writes a large number of sectors while profiling the time taken.
 * - Reads them back and profiles the time taken.
 */

#define CORTEX_M4
#include "navhal.h"

// Wait for a number of ms using systick
static void delay(uint32_t ms) { hal_delay_ms(ms); }

// Calculate performance in KB/s
static void print_perf(const char *op, uint32_t bytes, uint32_t ms) {
  uint32_t kb = bytes / 1024;

  if (ms == 0)
    ms = 1; // Prevent divide by zero

  uint32_t kb_per_sec = (kb * 1000) / ms;

  hal_uart_write_string(HAL_UART_2, op);
  hal_uart_write_string(HAL_UART_2, " Perf: ");
  hal_uart_write_uint(HAL_UART_2, kb_per_sec);
  hal_uart_write_string(HAL_UART_2, " KB/s (");
  hal_uart_write_uint(HAL_UART_2, ms);
  hal_uart_write_string(HAL_UART_2, " ms)\n\r");
}

#define TEST_SECTORS 4096 // 2MB
#define CHUNK_SIZE 128
#define TEST_START_SECTOR 2000 // Move away from start of disk

int main(void) {
  /* 1. Setup clocks and logging */
  hal_pll_config_t pll_cfg = {.input_src = HAL_CLOCK_SOURCE_HSI,
                              .pll_m = 16,
                              .pll_n = 336,
                              .pll_p = 4,
                              .pll_q = 7};
  hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

  hal_clock_init(&clk_cfg, &pll_cfg);
  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});

  delay(100);

  hal_uart_write_string(HAL_UART_2, "\n\r--- NavHAL SDIO Perf Test ---\n\r");
#ifdef _DMA_ENABLED
  hal_uart_write_string(HAL_UART_2, "DMA Mode: ENABLED\n\r");
#else
  hal_uart_write_string(HAL_UART_2, "DMA Mode: DISABLED (Polling)\n\r");
#endif

  /* 2. Initialize SDIO */
  hal_sdio_config_t sd_config = {.clock_div = 118,
                                 .bus_width = 1}; // 4-bit mode requested
  if (hal_sdio_init(&sd_config) != HAL_SDIO_OK) {
    hal_uart_write_string(HAL_UART_2, "SDIO Peripheral Init Failed!\n\r");
    while (1)
      ;
  }

  /* 3. Perform SD Card Handshake */
  hal_uart_write_string(HAL_UART_2, "Starting SD Card Handshake...\n\r");
  if (hal_sdio_card_init() != HAL_SDIO_OK) {
    hal_uart_write_string(HAL_UART_2, "SD Card Handshake Failed!\n\r");
    while (1)
      ;
  }
  hal_uart_write_string(HAL_UART_2, "SD Card Ready.\n\r");

  if (hal_disk_initialize(0) != HAL_DISK_STATUS_OK) {
    hal_uart_write_string(HAL_UART_2, "Disk Init Failed!\n\r");
    while (1)
      ;
  }
  hal_uart_write_string(HAL_UART_2, "Disk Initialized.\n\r");

  /* 4. Prepare Test Data */
  // Use a 64KB buffer (128 sectors) to test multi-block performance
  static uint8_t buf[CHUNK_SIZE * 512] __attribute__((aligned(4)));
  for (int i = 0; i < (CHUNK_SIZE * 512); i++) {
    buf[i] = (uint8_t)(i & 0xFF);
  }

  uint32_t start_time, end_time, elapsed;
  hal_disk_result_t res;

  /* 5. Write Performance Test */
  hal_uart_write_string(HAL_UART_2, "Starting Write Test (");
  hal_uart_write_uint(HAL_UART_2, TEST_SECTORS / 2);
  hal_uart_write_string(HAL_UART_2, " KB)...\n\r");

  start_time = hal_timebase_get_tick();

  // Note: Using a single block write loop to match current diskio
  // implementation. Multi-block commands would be much faster but
  // hal_disk_write currently uses a loop of single blocks.
  for (int i = 0; i < TEST_SECTORS; i += CHUNK_SIZE) {
    res = hal_disk_write(0, buf, TEST_START_SECTOR + i, CHUNK_SIZE);
    if (res != HAL_DISK_RES_OK)
      break;
  }

  end_time = hal_timebase_get_tick();
  elapsed = end_time - start_time;

  if (res == HAL_DISK_RES_OK) {
    hal_uart_write_string(HAL_UART_2, "Write Success.\n\r");
    print_perf("Write", TEST_SECTORS * 512, elapsed);
  } else {
    hal_uart_write_string(HAL_UART_2, "Write FAILED! Code: ");
    hal_uart_write_int(HAL_UART_2, (int)res);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  }

  /* 6. Read Performance Test */
  hal_uart_write_string(HAL_UART_2, "Starting Read Test...\n\r");

  start_time = hal_timebase_get_tick();

  for (int i = 0; i < TEST_SECTORS; i += CHUNK_SIZE) {
    res = hal_disk_read(0, buf, TEST_START_SECTOR + i, CHUNK_SIZE);
    if (res != HAL_DISK_RES_OK)
      break;
  }

  end_time = hal_timebase_get_tick();
  elapsed = end_time - start_time;

  if (res == HAL_DISK_RES_OK) {
    hal_uart_write_string(HAL_UART_2, "Read Success.\n\r");
    print_perf("Read", TEST_SECTORS * 512, elapsed);
  } else {
    hal_uart_write_string(HAL_UART_2, "Read FAILED! Code: ");
    hal_uart_write_int(HAL_UART_2, (int)res);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  }

  hal_uart_write_string(HAL_UART_2, "Test Complete.\n\r");

  while (1)
    ;
}
