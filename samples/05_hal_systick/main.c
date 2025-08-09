#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/uart.h"
#include "utils/clock_types.h"
#define CORTEX_M4
#include "navhal.h"

void delay() {
  for (volatile int i = 0; i < 100000; i++)
    ;
}

hal_pll_config_t pll_cfg = {.input_src =
                                HAL_CLOCK_SOURCE_HSE, // external crystal 8 MHz
                            .pll_m = 8,
                            .pll_n = 168,
                            .pll_p = 2,
                            .pll_q = 7};

hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE};

void print2(void) { uart2_write("Hello World 2\n\r"); }
void print3(void) { uart2_write("Hello World 3\n\r"); }
void print4(void) { uart2_write("Hello World 4\n\r"); }
void print5(void) { uart2_write("Hello World 5\n\r"); }
void print6(void) { uart2_write("Hello World 6\n\r"); }
void print7(void) { uart2_write("Hello World 7\n\r"); }
void print9(void) { uart2_write("Hello World 9\n\r"); }
void print12(void) { uart2_write("Hello World 12\n\r"); }

int main() {
  hal_clock_init(&cfg, &pll_cfg);
  systick_init(1000);
  uart2_init(9600);

  timer_init(TIM2, 128, 50000);
  timer_enable_interrupt(TIM2);
  timer_attach_callback(TIM2, print2);

  timer_init(TIM3, 128, 50000);
  timer_enable_interrupt(TIM3);
  timer_attach_callback(TIM3, print3);

  timer_init(TIM4, 128, 50000);
  timer_enable_interrupt(TIM4);
  timer_attach_callback(TIM4, print4);

  timer_init(TIM5, 128, 50000);
  timer_enable_interrupt(TIM5);
  timer_attach_callback(TIM5, print5);

  timer_init(TIM9, 128, 50000);
  timer_enable_interrupt(TIM9);
  timer_attach_callback(TIM9, print9);

  while (1) {
    /* uart2_write("SysTicks: "); */
    /* uart2_write(hal_get_tick()); */
    /* uart2_write(", Millis: "); */
    /* uart2_write(timer_get_count(TIM5)); */
    /* uart2_write("\n\r"); */
    /* // delay_ms(3); */
  }
}
