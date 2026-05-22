#define CORTEX_M4
#include "common/hal_features.h"
#include "navhal_port_fpu.h"
#include "navhal_port_uart.h"
#include "navhal.h"
#include "navtest/navtest.h"

#include "test_clock.h"
#include "test_crc.h"
#include "test_dma.h"
#include "test_dwt.h"
#include "test_flash_raw.h"
#include "test_fpu_accel.h"
#include "test_gpio.h"
#include "test_i2c.h"
#include "test_interrupt.h"
#include "test_pwm.h"
#include "test_sdio.h"
#include "test_spi.h"
#include "test_timebase.h"
#include "test_timer.h"
#include "test_uart_protocol.h"

static const navtest_suite_t *const all_suites[] = {
    &test_gpio_suite,
    &test_interrupt_suite,
    &test_timebase_suite,
    &test_timer_suite,
    &test_clock_suite,
    &test_pwm_suite,
#if NAVHAL_HAS_DMA
    &test_dma_suite,
#endif
    &test_crc_suite,
#if NAVHAL_HAS_CYCLE_COUNTER
    &test_dwt_suite,
#endif
#if NAVHAL_HAS_FPU
    &test_fpu_suite,
#endif
    &test_uart_protocol_suite,
    &test_i2c_suite,
    &test_spi_suite,
    &test_flash_suite,
#if NAVHAL_HAS_SDIO
    &test_sdio_suite,
#endif
};

static void print_startup_message(void) {
  hal_uart_write_char(HAL_UART_2, 0x1B); // ESC
  hal_uart_write_char(HAL_UART_2, '[');
  hal_uart_write_char(HAL_UART_2, '2');
  hal_uart_write_char(HAL_UART_2, 'J');

  hal_uart_write_char(HAL_UART_2, 0x1B); // ESC
  hal_uart_write_char(HAL_UART_2, '[');
  hal_uart_write_char(HAL_UART_2, 'H');
  const char *msg = "\r\n"
                    "|========================================|\r\n"
                    "|    NAVrobotec Private Limited          |\r\n"
                    "|          Project: NAVHAL               |\r\n"
                    "|     Starting Unit Tests...             |\r\n"
                    "|========================================|\r\n";

  for (const char *p = msg; *p != '\0'; p++) {
    hal_uart_write_char(HAL_UART_2, *p);
  }
}

int main(void) {
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
#if NAVHAL_HAS_FPU
  hal_fpu_enable();
#endif
  print_startup_message();

  int failed = 0;
  uint32_t total_tests = 0;
  const size_t n_suites = sizeof(all_suites) / sizeof(all_suites[0]);
  for (size_t i = 0; i < n_suites; i++) {
    failed += navtest_run_suite(all_suites[i]);
    total_tests += all_suites[i]->count;
  }

  hal_uart_print(HAL_UART_2, "\n\n=========== FINAL RESULTS ===========\n\n");
  hal_uart_print(HAL_UART_2, "Total tests run: ");
  _navtest_print_uint32(total_tests);
  hal_uart_print(HAL_UART_2, "\nTotal failures:  ");
  _navtest_print_uint32((uint32_t)failed);
  hal_uart_print(HAL_UART_2, "\n");

  return failed;
}
