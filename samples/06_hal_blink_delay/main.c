#define CORTEX_M4
#include "navhal.h"

// HAL PLL config for the clock
hal_pll_config_t pll_cfg = {.input_src =
                                HAL_CLOCK_SOURCE_HSE, // external crystal 8 MHz
                            .pll_m = 8,
                            .pll_n = 168,
                            .pll_p = 2,
                            .pll_q = 7};

// setting the clock config
//
hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE};

int main() {
  // Initialized the clock
  hal_clock_init(&cfg, &pll_cfg);
  // Start system tick with a interval of 40us
  systick_init(40);
  // Intialized the UART with a baud rate of 9600
  uart2_init(9600);
  // Setting GPIO PA05 as output and pull-up-down to none
  hal_gpio_setmode(GPIO_PA05, GPIO_OUTPUT, GPIO_PUPD_NONE);
  // Initlzed the TIMER5 with a prescaler of 0 and a restart count of  1000000000
  timer_init(TIM5, 0, 1000000000);
  // Infinite update loop
  while (1) {
    // Set the GPIO at PA05 as HIGH
    hal_gpio_digitalwrite(GPIO_PA05, GPIO_HIGH);
    // Delay the system with 500ms ~ it uses the systicks for it
    delay_ms(500);
    // Set the GPIO at PA05 as HIGH
    hal_gpio_digitalwrite(GPIO_PA05, GPIO_LOW);
    // Delay the system with 500ms ~ it uses the systicks for it
    delay_ms(500);
    uart2_write("Millis: "); // Printing millis to the console
    uart2_write(hal_get_millis()); // Getting and printing millis to the screen
    uart2_write("\n\r"); // Break the line and return carriage
    // delay_ms(500);
  }
}
