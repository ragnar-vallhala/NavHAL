#define CORTEX_M4
#include "navhal.h"
#include "test_gpio.h"
#include "test_timer.h"
#include "unity.h"
void setUp(void) {}
void tearDown(void) {}

static int total_test_count = 0;

int test_gpio(void) {
  uart2_write("\n===========GPIO TEST START===========\n\n");
  UNITY_BEGIN();
  RUN_TEST(test_hal_gpio_setmode);
  RUN_TEST(test_hal_gpio_getmode);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_high);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_low);
  RUN_TEST(test_gpio_set_alternate_function);
  uart2_write("\n===========GPIO TEST END===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

int test_timer(void) {

  uart2_write("\n===========TIMER TEST START===========\n\n");
  UNITY_BEGIN();
  RUN_TEST(test_timer_init_sets_prescaler_and_arr);
  RUN_TEST(test_timer_start_sets_CEN_bit);
  RUN_TEST(test_timer_stop_clears_CEN_bit);
  RUN_TEST(test_timer_reset_clears_count);
  RUN_TEST(test_timer_set_compare_and_get_compare);
  RUN_TEST(test_timer_enable_and_disable_channel);
  // hangs the UART
  // RUN_TEST(test_timer_enable_and_disable_interrupt);
  RUN_TEST(test_timer_clear_interrupt_flag_clears_UIF);
  RUN_TEST(test_timer_get_arr_returns_arr_value);
  RUN_TEST(test_timer_get_count_returns_count_value);
  uart2_write("\n===========TIMER TEST END===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}
#include "core/cortex-m4/uart.h"

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
  uart2_write("\n\n===========FINAL RESULTS===========\n\n");
  uart2_write(total_test_count);
  uart2_write(" Tests | ");
  uart2_write(failed);
  uart2_write(" Failures\n");
  return failed;
}
