#define CORTEX_M4
#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/uart_reg.h"
#include "navhal.h"
#include "navtest/navtest.h"
#include "test_clock.h"
#include "test_gpio.h"
#include "test_pwm.h"
#include "test_timer.h"
#ifdef _DMA_ENABLED
#include "test_dma.h"
#endif
#include "core/cortex-m4/fpu.h"
#include "test_crc.h"
#include "test_dwt.h"

static void wait_uart_empty(void) {
  volatile UARTx_Reg_Typedef *uart2 =
      (volatile UARTx_Reg_Typedef *)(GET_USARTx_BASE(2));
  while (!(uart2->SR & USART_SR_TC))
    ;
}

static int total_test_count = 0;

int test_gpio(void) {
  uart2_write("\n=========== GPIO TEST START ===========\n");
  NAVTEST_BEGIN();
  RUN_TEST(test_hal_gpio_setmode);
  RUN_TEST(test_hal_gpio_getmode);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_high);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_low);
  RUN_TEST(test_gpio_set_alternate_function);
  uart2_write("=========== GPIO TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

int test_timer(void) {
  uart2_write("\n=========== TIMER TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_timer_init_sets_prescaler_and_arr);
  RUN_TEST(test_timer_start_sets_CEN_bit);
  RUN_TEST(test_timer_stop_clears_CEN_bit);
  RUN_TEST(test_timer_reset_clears_count);
  RUN_TEST(test_timer_set_compare_and_get_compare);
  RUN_TEST(test_timer_enable_and_disable_channel);
  // RUN_TEST(test_timer_enable_and_disable_interrupt); // hangs UART
  RUN_TEST(test_timer_clear_interrupt_flag_clears_UIF);
  RUN_TEST(test_timer_set_arr_and_get_arr);
  RUN_TEST(test_timer_get_count_returns_correct_value);
  RUN_TEST(test_systick_tick_increments);
  RUN_TEST(test_timer_get_frequency_returns_correct_value);

  uart2_write("=========== TIMER TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

int test_clock(void) {
  uart2_write("\n=========== CLOCK TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_hal_clock_init_hsi);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_init_hse);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_init_pll);
  wait_uart_empty();

  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_hsi);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_hse);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_pll);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_get_ahbclk_returns_correct_value);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_get_apb1clk_returns_correct_value);
  wait_uart_empty();
  RUN_TEST(test_hal_clock_get_apb2clk_returns_correct_value);
  for (volatile int i = 0; i < 10000; i++)
    __asm__("nop"); // Small delay to let UART finish

  uart2_write("=========== CLOCK TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

// -------------------- PWM --------------------
int test_pwm(void) {
  uart2_write("\n=========== PWM TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_hal_pwm_init_apb1);
  RUN_TEST(test_hal_pwm_init_apb2);
  RUN_TEST(test_hal_pwm_start_sets_counter_enable);
  RUN_TEST(test_hal_pwm_stop_clears_counter_enable);
  RUN_TEST(test_hal_pwm_set_duty_cycle_updates_ccr);

  uart2_write("=========== PWM TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

#ifdef _DMA_ENABLED
// -------------------- DMA --------------------
int test_dma_suite(void) {
  uart2_write("\n=========== DMA TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_dma_clock_enable_dma1);
  RUN_TEST(test_dma_clock_enable_dma2);
  RUN_TEST(test_dma_init_sets_channel);
  RUN_TEST(test_dma_init_sets_direction_m2p);
  RUN_TEST(test_dma_init_sets_direction_p2m);
  RUN_TEST(test_dma_init_sets_minc);
  RUN_TEST(test_dma_init_sets_priority);
  RUN_TEST(test_dma_init_sets_ndtr);
  RUN_TEST(test_dma_init_sets_peripheral_address);
  RUN_TEST(test_dma_init_sets_memory_address);
  RUN_TEST(test_dma_start_enables_stream);
  RUN_TEST(test_dma_stop_disables_stream);
  RUN_TEST(test_dma_transfer_complete_returns_zero_before_start);
  RUN_TEST(test_dma_clear_flags_clears_isr);

  uart2_write("=========== DMA TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}
#endif

// -------------------- CRC --------------------
int test_crc_suite(void) {
  uart2_write("\n=========== CRC TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_crc_empty_returns_init);
  RUN_TEST(test_crc_single_byte);
  RUN_TEST(test_crc_known_vector);
  RUN_TEST(test_crc_accumulate_matches_compute);
  RUN_TEST(test_crc_reset_restores_init);

  uart2_write("=========== CRC TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

// -------------------- DWT --------------------
int test_dwt_suite(void) {
  uart2_write("\n=========== DWT TEST START ===========\n");
  NAVTEST_BEGIN();

  RUN_TEST(test_dwt_init_enables_counters);
  RUN_TEST(test_dwt_get_cycles_increments);
  RUN_TEST(test_dwt_reset_cycles_zeros_counter);
  RUN_TEST(test_dwt_delay_cycles_elapses_time);

  uart2_write("=========== DWT TEST END ===========\n");
  total_test_count += (int)navtest_get_test_count();
  return NAVTEST_END();
}

void print_startup_message(void) {
  uart2_write_char(0x1B); // ESC
  uart2_write_char('[');
  uart2_write_char('2');
  uart2_write_char('J');

  uart2_write_char(0x1B); // ESC
  uart2_write_char('[');
  uart2_write_char('H');
  const char *msg = "\r\n"
                    "|========================================|\r\n"
                    "|    NAVrobotec Private Limited          |\r\n"
                    "|          Project: NAVHAL               |\r\n"
                    "|     Starting Unit Tests...             |\r\n"
                    "|========================================|\r\n";

  for (const char *p = msg; *p != '\0'; p++) {
    uart2_write_char(*p);
  }
}

int main(void) {
  uart2_init(9600);
  hal_fpu_enable();
  print_startup_message();

  int failed = 0;
  failed += test_gpio();
  failed += test_timer();
  failed += test_clock();
  failed += test_pwm();
#ifdef _DMA_ENABLED
  failed += test_dma_suite();
#endif
  failed += test_crc_suite();
  failed += test_dwt_suite();

  uart2_write("\n\n=========== FINAL RESULTS ===========\n\n");
  uart2_write("Total tests run: ");
  /* reuse navtest's uint32 printer indirectly via a temp group */
  {
    /* print total_test_count manually */
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    int v = total_test_count;
    if (v == 0) {
      buf[--i] = '0';
    }
    while (v && i > 0) {
      buf[--i] = '0' + (v % 10);
      v /= 10;
    }
    uart2_write(buf + i);
  }
  uart2_write("\nTotal failures:  ");
  {
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    int v = failed;
    if (v == 0) {
      buf[--i] = '0';
    }
    while (v && i > 0) {
      buf[--i] = '0' + (v % 10);
      v /= 10;
    }
    uart2_write(buf + i);
  }
  uart2_write("\n");

  return failed;
}
