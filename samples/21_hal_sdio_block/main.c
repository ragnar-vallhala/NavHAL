/**
 * @file main.c
 * @brief Example: Test SDIO Block Read/Write using hal_diskio.
 *
 * @details
 * - Initializes PLL (84MHz) and SDIO.
 * - Writes a 512-byte pattern to Sector 1.
 * - Reads it back and verifies the data integrity.
 */

#define CORTEX_M4
#include "navhal.h"

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

  hal_uart_write_string(HAL_UART_2, "\n\r--- NavHAL SDIO Block Test ---\n\r");

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
  hal_uart_write_string(HAL_UART_2, "SD Card Ready (Transfer State).\n\r");

  if (hal_disk_initialize(0) != HAL_DISK_STATUS_OK) {
    hal_uart_write_string(HAL_UART_2, "Disk Init Failed!\n\r");
    while (1)
      ;
  }
  hal_uart_write_string(HAL_UART_2, "Disk Initialized.\n\r");

  /* 4. Prepare Test Data */
  uint8_t write_buf[512] __attribute__((aligned(4)));
  uint8_t read_buf[512] __attribute__((aligned(4)));
  for (int i = 0; i < 512; i++)
    write_buf[i] = (uint8_t)i;
  for (int i = 0; i < 512; i++)
    read_buf[i] = 0;

  /* 5. Write to Sector 100 (Arbitrary safe sector) */
  hal_uart_write_string(HAL_UART_2, "Writing 512 bytes to Sector 100...\n\r");
  hal_disk_result_t res = hal_disk_write(0, write_buf, 100, 1);
  if (res == HAL_DISK_RES_OK) {
    hal_uart_write_string(HAL_UART_2, "Write Success.\n\r");
  } else {
    hal_uart_write_string(HAL_UART_2, "Write FAILED! Code: ");
    hal_uart_print(HAL_UART_2, (int)res);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  }

  /* 6. Read back from Sector 100 */
  hal_uart_write_string(HAL_UART_2, "Reading 512 bytes from Sector 100...\n\r");
  res = hal_disk_read(0, read_buf, 100, 1);
  if (res == HAL_DISK_RES_OK) {
    hal_uart_write_string(HAL_UART_2, "Read Success.\n\r");
  } else {
    hal_uart_write_string(HAL_UART_2, "Read FAILED! Code: ");
    hal_uart_print(HAL_UART_2, (int)res);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  }

  /* 7. Verify Data */
  int errors = 0;
  for (int i = 0; i < 512; i++) {
    if (read_buf[i] != write_buf[i])
      errors++;
  }

  if (errors == 0) {
    hal_uart_write_string(HAL_UART_2, "Verification PASSED! Data matches perfectly.\n\r");
  } else {
    hal_uart_write_string(HAL_UART_2, "Verification FAILED! Bytes mismatch: ");
    hal_uart_print(HAL_UART_2, errors);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  }

  while (1)
    ;
}
