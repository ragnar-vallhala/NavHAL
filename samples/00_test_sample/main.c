#define CORTEX_M4
#include "navhal.h"

int main() {
  uart2_init(9600);
  systick_init(1000);
  uart2_write("Version: ");
  uart2_write(VERSION);
  uart2_write("\n");
  timer_init(TIM1, 1024, 32000);
  timer_init(TIM2, 1024, 32000);
  timer_init(TIM3, 1024, 32000);
  timer_init(TIM4, 1024, 32000);
  timer_init(TIM5, 1024, 32000);
  timer_init(TIM6, 1024, 32000);
  timer_init(TIM7, 1024, 32000);
  timer_init(TIM8, 1024, 32000);
  timer_init(TIM9, 1024, 32000);
  timer_init(TIM10, 1024, 32000);
  timer_init(TIM11, 1024, 32000);
  // timer_start(TIM3);
  while (1) {
    uart2_write("TIM1: ");
    uart2_write(timer_get_count(TIM1));
    uart2_write(", TIM2: ");
    uart2_write(timer_get_count(TIM2));
    uart2_write(", TIM3: ");
    uart2_write(timer_get_count(TIM3));
    uart2_write(", TIM4: ");
    uart2_write(timer_get_count(TIM4));
    uart2_write(", TIM5: ");
    uart2_write(timer_get_count(TIM5));
    uart2_write(", TIM6: ");
    uart2_write(timer_get_count(TIM6));
    uart2_write(", TIM7: ");
    uart2_write(timer_get_count(TIM7));
    uart2_write(", TIM8: ");
    uart2_write(timer_get_count(TIM8));
    uart2_write(", TIM9: ");
    uart2_write(timer_get_count(TIM9));
    uart2_write(", TIM10: ");
    uart2_write(timer_get_count(TIM10));
    uart2_write(", TIM11: ");
    uart2_write(timer_get_count(TIM11));
    uart2_write(", Ticks: ");
    uart2_write(hal_get_tick());
    uart2_write("\n");
    delay_ms(100);
  }
}
