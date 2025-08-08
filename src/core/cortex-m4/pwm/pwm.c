#include "core/cortex-m4/pwm.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/timer.h"
#include <stdint.h>

void hal_pwm_init(PWM_Handle *pwm, uint32_t frequency, float dutyCycle) {

  // 1. Get clock
  uint32_t bus_clk = hal_clock_get_apb1clk(); // default for TIM2-TIM5
  if (pwm->timer == TIM1 || pwm->timer == TIM9 || pwm->timer == TIM10 ||
      pwm->timer == TIM11) {
    bus_clk = hal_clock_get_apb2clk(); // For advanced timers
  }
  uint32_t psc = bus_clk / 1e6 - 1;
  uint32_t arr = (bus_clk / (psc + 1)) / frequency - 1;
  uint32_t ccr = (uint32_t)((arr + 1) * dutyCycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  timer_init(pwm->timer, psc, arr);
  timer_set_compare(pwm->timer, pwm->channel, ccr);
}

void hal_pwm_start(PWM_Handle *pwm) { timer_start(pwm->timer); }

void hal_pwm_stop(PWM_Handle *pwm) {
  timer_disable_channel(pwm->timer, pwm->channel);
  timer_stop(pwm->timer);
}

void hal_pwm_set_duty_cycle(PWM_Handle *pwm, float dutyCycle) {
  timer_set_compare(pwm->timer, pwm->channel, dutyCycle);
}

void hal_pwm_set_frequency(PWM_Handle *pwm, uint32_t frequency) {
  /* Timer_SetFrequency(&pwm->timer, frequency); */
}
