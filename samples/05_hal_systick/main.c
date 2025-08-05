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
int main() {
  hal_clock_init(&cfg, &pll_cfg);
  systick_init(1000);
  uart2_init(9600);
  timer_init(TIM5, 0, 1000000000);
  while (1) {
    uart2_write("SysTicks: ");
    uart2_write(hal_get_tick());
    uart2_write(", Millis: ");
    uart2_write(hal_get_millis());
    uart2_write("\n\r");
    // delay_ms(500);
  }
}
