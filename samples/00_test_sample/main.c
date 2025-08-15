#define CORTEX_M4
#include "navhal.h"

hal_pll_config_t pll_cfg;

hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI};

void print2(void) { uart2_write("Hello World 2\n\r"); }
void print3(void) { uart2_write("Hello World 3\n\r"); }
void print4(void) { uart2_write("Hello World 4\n\r"); }
void print5(void) { uart2_write("Hello World 5\n\r"); }
void print6(void) { uart2_write("Hello World 6\n\r"); }
void print7(void) { uart2_write("Hello World 7\n\r"); }
void print9(void) { uart2_write("Hello World 9\n\r"); }
// input clock = 16MHz
// Prescaler = 500 so oFreq = 32KHz
// Arr = 3200 so callbacks are called each sec
int main() {
  hal_clock_init(&cfg, &pll_cfg);
  systick_init(1000);
  uart2_init(9600);

  timer_init(TIM2, 500, 32000);
  timer_enable_interrupt(TIM2);
  timer_attach_callback(TIM2, print2);

  timer_init(TIM3, 500, 32000);
  timer_enable_interrupt(TIM3);
  timer_attach_callback(TIM3, print3);

  timer_init(TIM4, 500, 32000);
  timer_enable_interrupt(TIM4);
  timer_attach_callback(TIM4, print4);

  timer_init(TIM5, 500, 32000);
  timer_enable_interrupt(TIM5);
  timer_attach_callback(TIM5, print5);

  timer_init(TIM9, 500, 32000);
  timer_enable_interrupt(TIM9);
  timer_attach_callback(TIM9, print9);

  while (1)
    ;
}
