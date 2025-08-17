#define CORTEX_M4
#include "core/cortex-m4/uart.h"
#include "navhal.h"
#include "test_clock.h"
#include "test_gpio.h"
#include "test_timer.h"
#include "test_pwm.h"   // ⬅️ added
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static int total_test_count = 0;

int test_gpio(void) {
  uart2_write("\n=========== GPIO TEST START ===========\n");
  UNITY_BEGIN();
  RUN_TEST(test_hal_gpio_setmode);
  RUN_TEST(test_hal_gpio_getmode);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_high);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_low);
  RUN_TEST(test_gpio_set_alternate_function);
  uart2_write("=========== GPIO TEST END ===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

int test_timer(void) {
  uart2_write("\n=========== TIMER TEST START ===========\n");
  UNITY_BEGIN();

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
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

int test_clock(void) {
  uart2_write("\n=========== CLOCK TEST START ===========\n");
  UNITY_BEGIN();

  RUN_TEST(test_hal_clock_init_hsi);
  RUN_TEST(test_hal_clock_init_hse);
  RUN_TEST(test_hal_clock_init_pll);

  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_hsi);
  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_hse);
  RUN_TEST(test_hal_clock_get_sysclk_returns_correct_value_pll);

  RUN_TEST(test_hal_clock_get_ahbclk_returns_correct_value);
  RUN_TEST(test_hal_clock_get_apb1clk_returns_correct_value);
  RUN_TEST(test_hal_clock_get_apb2clk_returns_correct_value);

  uart2_write("=========== CLOCK TEST END ===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

// -------------------- PWM --------------------
int test_pwm(void) {
  uart2_write("\n=========== PWM TEST START ===========\n");
  UNITY_BEGIN();

  RUN_TEST(test_hal_pwm_init_apb1);
  RUN_TEST(test_hal_pwm_init_apb2);
  RUN_TEST(test_hal_pwm_start_sets_counter_enable);
  RUN_TEST(test_hal_pwm_stop_clears_counter_enable);
  RUN_TEST(test_hal_pwm_set_duty_cycle_updates_ccr);

  uart2_write("=========== PWM TEST END ===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
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
  print_startup_message();

  int failed = 0;
  failed += test_gpio();
  failed += test_timer();
  failed += test_clock();
  failed += test_pwm();   // ⬅️ run PWM tests

  uart2_write("\n\n=========== FINAL RESULTS ===========\n\n");
  uart2_write(total_test_count);
  uart2_write(" Tests | ");
  uart2_write(failed);
  uart2_write(" Failures\n");

  return failed;
}

