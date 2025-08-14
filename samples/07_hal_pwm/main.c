#define CORTEX_M4
#include "navhal.h"

int main()
{
  // Start system tick with a interval of 40us
  systick_init(40);
  uart2_init(9600);
  hal_gpio_setmode(GPIO_PB10, GPIO_AF, GPIO_PUPD_NONE);
  hal_gpio_set_alternate_function(GPIO_PB10, GPIO_AF01);

  // Initlzed the TIMER2 with a prescaler of 0 and a restart count of 1000000000
  PWM_Handle pwm;
  pwm.timer = TIM2;
  pwm.channel = 3;

  hal_pwm_init(&pwm, 15000, 0.1000);
  hal_pwm_start(&pwm);
  float value = 0;
  // Infinite update loop
  while (1)
  {
    hal_pwm_set_duty_cycle(&pwm, value);
    value += 0.01;
    if (value >= 1)
    {
      value = 0;
    }
    uart2_write(value); // Getting and printing millis to the screen

    uart2_write("\n\r"); // Break the line and return carriage */
    delay_ms(10);
  }
}
