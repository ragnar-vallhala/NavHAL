#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/pwm.h"
#include "core/cortex-m4/timer.h"
#include "utils/gpio_types.h"
#define CORTEX_M4
#include "navhal.h"

int main() {
  // Start system tick with a interval of 40us
  systick_init(1000);
  uart2_init(9600);
  hal_gpio_setmode(GPIO_PB10, GPIO_AF, GPIO_PUPD_NONE);
  hal_gpio_set_alternate_function(GPIO_PB10, GPIO_AF01);

  // Initlzed the TIMER5 with a prescaler of 0 and a restart count of 1000000000
  PWM_Handle pwm;
  pwm.timer = TIM2;
  pwm.channel = 3;

  hal_pwm_init(&pwm, 15000, 0.356);
  hal_pwm_start(&pwm);

  // Infinite update loop
  while (1) {
    /* uart2_write( */
    /*     timer_get_count(TIM2)); // Getting and printing millis to the screen
     */

    /* uart2_write("\n\r"); // Break the line and return carriage */
    /* delay_ms(100); */
  }
}
