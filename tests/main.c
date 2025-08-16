/**
 * @file main.c
 * @brief Unit test runner for NAVHAL Cortex-M4 hardware abstraction layer
 * @details
 * This file implements the main test runner for the NAVHAL unit tests.
 * It includes:
 * - Test setup/teardown functions
 * - Individual test suite runners (GPIO, Timer)
 * - Test result reporting
 * - ANSI terminal control for output formatting
 *
 * Tests are implemented using the Unity test framework and output results
 * via UART2 at 9600 baud.
 *
 * @note Requires UART2 to be properly configured before running
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"
#include "test_gpio.h"
#include "test_timer.h"
#include "unity.h"

/**
 * @brief Unity test framework setup function
 * @details Empty implementation since no special setup is required
 */
void setUp(void) {}

/**
 * @brief Unity test framework teardown function
 * @details Empty implementation since no special cleanup is required
 */
void tearDown(void) {}

static int total_test_count = 0; ///< Tracks total number of tests executed

/**
 * @brief Run all GPIO tests
 * @return Number of failed tests
 * @details
 * Executes the following test cases:
 * - GPIO mode setting/reading
 * - Digital write operations
 * - Alternate function configuration
 */
int test_gpio(void) {
  uart2_write("\n===========GPIO TEST START===========\n");
  UNITY_BEGIN();
  RUN_TEST(test_hal_gpio_setmode);
  RUN_TEST(test_hal_gpio_getmode);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_high);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_low);
  RUN_TEST(test_gpio_set_alternate_function);
  uart2_write("===========GPIO TEST END===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

/**
 * @brief Run all Timer tests
 * @return Number of failed tests
 * @details
 * Executes the following test cases:
 * - Timer initialization
 * - Start/stop functionality
 * - Counter operations
 * - Compare register functionality
 * - Channel enable/disable
 */
int test_timer(void) {
  uart2_write("\n===========TIMER TEST START===========\n");
  UNITY_BEGIN();
  RUN_TEST(test_timer_init_sets_prescaler_and_arr);
  RUN_TEST(test_timer_start_sets_CEN_bit);
  RUN_TEST(test_timer_stop_clears_CEN_bit);
  RUN_TEST(test_timer_reset_clears_count);
  RUN_TEST(test_timer_set_compare_and_get_compare);
  RUN_TEST(test_timer_enable_and_disable_channel);
  RUN_TEST(test_timer_clear_interrupt_flag_clears_UIF);
  RUN_TEST(test_timer_get_arr_returns_arr_value);
  RUN_TEST(test_timer_get_count_returns_count_value);
  uart2_write("===========TIMER TEST END===========\n");
  total_test_count += Unity.NumberOfTests;
  return UNITY_END();
}

/**
 * @brief Print startup banner to UART
 * @details
 * Clears terminal screen and prints formatted startup message
 * using ANSI escape sequences
 */
void print_startup_message(void) {
  // Send ANSI clear screen and home cursor commands
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

/**
 * @brief Main test runner function
 * @return Total number of failed tests
 * @details
 * Initializes UART2, prints startup message, runs all test suites,
 * and reports final results
 */
int main(void) {
  // Initialize UART and display startup message
  uart2_init(9600);
  print_startup_message();

  // Run test suites and accumulate failures
  int failed = 0;
  failed += test_gpio();
  failed += test_timer();

  // Print final results
  uart2_write("\n\n===========FINAL RESULTS===========\n\n");
  uart2_write_int(total_test_count);
  uart2_write(" Tests | ");
  uart2_write_int(failed);
  uart2_write(" Failures\n");
  
  return failed;
}