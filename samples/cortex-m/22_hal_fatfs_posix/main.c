/**
 * @file main.c
 * @brief Example: Test FatFS and POSIX-like API.
 */

#include "navhal_port_timer.h"
#include <stdint.h>
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
  hal_timebase_init(1000);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});

  hal_uart_write_string(HAL_UART_2, "\n\r--- NavHAL FatFS/POSIX Test ---\n\r");

  /* 2. Initialize SDIO */
  hal_sdio_config_t sd_config = {.clock_div = 118, .bus_width = 1};
  if (hal_sdio_init(&sd_config) != HAL_SDIO_OK) {
    hal_uart_write_string(HAL_UART_2, "SDIO Peripheral Init Failed!\n\r");
    while (1)
      ;
  }

  /* 3. Perform SD Card Handshake */
  if (hal_sdio_card_init() != HAL_SDIO_OK) {
    hal_uart_write_string(HAL_UART_2, "SD Card Handshake Failed!\n\r");
    while (1)
      ;
  }
  hal_uart_write_string(HAL_UART_2, "SD Card Ready.\n\r");

  /* 4. Initialize Filesystem */
  if (v_fs_init() != 0) {
    hal_uart_write_string(HAL_UART_2, "Filesystem Mount Failed!\n\r");
    while (1)
      ;
  }
  hal_uart_write_string(HAL_UART_2, "Filesystem Mounted.\n\r");

  /* 5. Create and write to a file */
  hal_uart_write_string(HAL_UART_2, "Creating test.txt...\n\r");
  uint32_t size = 1 * 1024;
  hal_timer_init_freq(TIM1, 1000000);
  v_fd_t fd;

  for (; size < 1024 * 10; size <<= 1) {
    fd = v_open("test.txt", V_O_CREAT | V_O_RDWR | V_O_TRUNC);
    if (fd < 0) {
      hal_uart_write_string(HAL_UART_2, "Failed to open test.txt for writing.\n\r");
      while (1)
        ;
    }
    char str[size];
    hal_memset(str, 0, sizeof(str));
    for (int i = 0; i < size; i++) {
      str[i] = i;
    }
    hal_timer_start(TIM1);
    hal_timer_reset(TIM1);
    int written = v_write(fd, str, size);
    hal_timer_stop(TIM1);
    uint32_t time = hal_timer_get_count(TIM1);
    hal_uart_write_string(HAL_UART_2, "Write Time: ");
    hal_uart_print(HAL_UART_2, time);
    hal_uart_write_string(HAL_UART_2, " ticks\n\r");
    if (written > 0) {
      hal_uart_write_string(HAL_UART_2, "Write Success (");
      hal_uart_print(HAL_UART_2, written);
      hal_uart_write_string(HAL_UART_2, " bytes).\n\r");
    } else {
      hal_uart_write_string(HAL_UART_2, "Write Error (Code: ");
      hal_uart_print(HAL_UART_2, -written);
      hal_uart_write_string(HAL_UART_2, ").\n\r");
    }
    v_sync(fd);
    v_close(fd);
  }

  /* 6. Read back from the file */
  hal_uart_write_string(HAL_UART_2, "Reading back test.txt...\n\r");
  fd = v_open("test.txt", V_O_RDONLY);
  if (fd < 0) {
    hal_uart_write_string(HAL_UART_2, "Failed to open test.txt for reading.\n\r");
    while (1)
      ;
  }

  uint8_t read_buf[1024 * 6] __attribute__((aligned(4)));
  hal_memset(read_buf, 0, sizeof(read_buf));
  int read_bytes = v_read(fd, read_buf, 1024 * 6);
  if (read_bytes > 0) {
    hal_uart_write_string(HAL_UART_2, "Read Success (");
    hal_uart_print(HAL_UART_2, read_bytes);
    hal_uart_write_string(HAL_UART_2, " bytes).\n\r");
    hal_uart_write_string(HAL_UART_2, "Data: ");
    hal_uart_write_string(HAL_UART_2, (char *)read_buf);
    hal_uart_write_string(HAL_UART_2, "\n\r");
  } else {
    hal_uart_write_string(HAL_UART_2, "Read Error (Code: ");
    hal_uart_print(HAL_UART_2, -read_bytes);
    hal_uart_write_string(HAL_UART_2, ").\n\r");
  }
  v_close(fd);

  hal_uart_write_string(HAL_UART_2, "Test Finished Success.\n\r");

  while (1)
    ;
}
