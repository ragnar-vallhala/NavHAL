/**
 * @file main.c
 * @brief Example: Test FatFS and POSIX-like API.
 */

#define CORTEX_M4
#include "navhal.h"
#include "utils/util.h"
#include "utils/v_fs.h"

int main(void) {
  /* 1. Setup clocks and logging */
  hal_pll_config_t pll_cfg = {.input_src = HAL_CLOCK_SOURCE_HSI,
                              .pll_m = 16,
                              .pll_n = 336,
                              .pll_p = 4,
                              .pll_q = 7};
  hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

  hal_clock_init(&clk_cfg, &pll_cfg);
  systick_init(1000);
  uart2_init(115200);

  uart2_write_string("\n\r--- NavHAL FatFS/POSIX Test ---\n\r");

  /* 2. Initialize SDIO */
  hal_sdio_config_t sd_config = {.clock_div = 118, .bus_width = 1};
  if (sdio_init(&sd_config) != HAL_SDIO_OK) {
    uart2_write_string("SDIO Peripheral Init Failed!\n\r");
    while (1)
      ;
  }

  /* 3. Perform SD Card Handshake */
  if (sdio_card_init() != HAL_SDIO_OK) {
    uart2_write_string("SD Card Handshake Failed!\n\r");
    while (1)
      ;
  }
  uart2_write_string("SD Card Ready.\n\r");

  /* 4. Initialize Filesystem */
  if (v_fs_init() != 0) {
    uart2_write_string("Filesystem Mount Failed!\n\r");
    while (1)
      ;
  }
  uart2_write_string("Filesystem Mounted.\n\r");

  /* 5. Create and write to a file */
  uart2_write_string("Creating test.txt...\n\r");
  v_fd_t fd = v_open("test.txt", V_O_CREAT | V_O_RDWR | V_O_TRUNC);
  if (fd < 0) {
    uart2_write_string("Failed to open test.txt for writing.\n\r");
    while (1)
      ;
  }

  const char *msg = "Hello from NavHAL FatFS!";
  int written = v_write(fd, msg, 24);
  if (written > 0) {
    uart2_write_string("Write Success (");
    uart2_write(written);
    uart2_write_string(" bytes).\n\r");
  } else {
    uart2_write_string("Write Error (Code: ");
    uart2_write(-written);
    uart2_write_string(").\n\r");
  }
  v_close(fd);

  /* 6. Read back from the file */
  uart2_write_string("Reading back test.txt...\n\r");
  fd = v_open("test.txt", V_O_RDONLY);
  if (fd < 0) {
    uart2_write_string("Failed to open test.txt for reading.\n\r");
    while (1)
      ;
  }

  uint8_t read_buf[32] __attribute__((aligned(4)));
  hal_memset(read_buf, 0, sizeof(read_buf));
  int read_bytes = v_read(fd, read_buf, 25);
  if (read_bytes > 0) {
    uart2_write_string("Read Success (");
    uart2_write(read_bytes);
    uart2_write_string(" bytes).\n\r");
    uart2_write_string("Data: ");
    uart2_write_string((char *)read_buf);
    uart2_write_string("\n\r");
  } else {
    uart2_write_string("Read Error (Code: ");
    uart2_write(-read_bytes);
    uart2_write_string(").\n\r");
  }
  v_close(fd);

  uart2_write_string("Test Finished Success.\n\r");

  while (1)
    ;
}
