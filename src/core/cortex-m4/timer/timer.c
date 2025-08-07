#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/interrupt.h"
#include "core/cortex-m4/uart.h"
#include "utils/timer_types.h"
#include <stdint.h>

static volatile uint64_t systick_ticks = 0;     // global ticks
static volatile uint32_t tick_duration_us = 1;  // global tick duration in us
static volatile uint32_t tick_reload_value = 0; // ticks relaod value

void systick_init(uint32_t tick_us) {
  // systick interrupt is not under the NVIC
  tick_duration_us = tick_us;
  uint32_t ahbclk = hal_clock_get_ahbclk();
  uint64_t reload_value = ((uint64_t)ahbclk / 1000000ULL) * tick_us - 1;
  reload_value &= 0xffffff; // set the reload value as 24bit only
  uint32_t reload = (uint32_t)reload_value;
  tick_reload_value = reload;
  SYST_RVR = reload; // Set reload value
  SYST_CVR = 0;      // Clear current value
  SYST_CSR =
      (1 << SYST_CSR_EN_BIT) | (1 << SYST_CSR_TICKINT_BIT) |
      (1 << SYST_CSR_CLKSOURCE_BIT); // Enable | TickInt | ClkSource
                                     // CLOCK source 0 means the systick run at
                                     // ahbclk/8 and if it's 1 it runs at ahbclk
}

void delay_us(uint64_t us) {
  volatile uint64_t ticks_needed = us / hal_get_tick_duration_us();
  if (ticks_needed ==
      0) // if 0 ticks are needed for the delay raise it to 1 tick
    ticks_needed = 1;
  volatile uint32_t start = hal_get_tick();
  while (hal_get_tick() - start < ticks_needed)
    __asm__ volatile("nop"); // insert noops in bw
  ;
}

void delay_ms(uint32_t ms) { delay_us(ms * 1000); }

uint64_t hal_get_tick(void) { return systick_ticks; }
uint32_t hal_get_tick_duration_us(void) { return tick_duration_us; }
uint32_t hal_get_tick_reload_value(void) { return tick_reload_value; }

uint32_t hal_get_millis(void) { return hal_get_micros() / 1000; }
uint32_t hal_get_micros(void) {
  return hal_get_tick() * hal_get_tick_duration_us();
}
void SysTick_Handler(void) { systick_ticks++; }
uint32_t _get_timer_base(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    return TIM1_BASE;
  case TIM2:
    return TIM2_BASE;
  case TIM3:
    return TIM3_BASE;
  case TIM4:
    return TIM4_BASE;
  case TIM5:
    return TIM5_BASE;
  case TIM9:
    return TIM9_BASE;
  case TIM10:
    return TIM10_BASE;
  case TIM11:
    return TIM11_BASE;
  default:
    return 0;
  }
}

void _enable_timer_rcc(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM1_OFFSET);
    break;
  case TIM2:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM2_OFFSET);
    break;
  case TIM3:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM3_OFFSET);
    break;
  case TIM4:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM4_OFFSET);
    break;
  case TIM5:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM5_OFFSET);
    break;
  case TIM9:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM9_OFFSET);
    break;
  case TIM10:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM10_OFFSET);
    break;
  case TIM11:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM11_OFFSET);
    break;
  default:
    break;
  }
}

void _timer_gp1_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_GP1_CR1_CEN_BIT));

  volatile uint32_t *timx_psc =
      (volatile uint32_t *)(timer_base + TIM_GP1_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint32_t *timx_cnt =
      (volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;

  volatile uint32_t *timx_arr =
      (volatile uint32_t *)(timer_base + TIM_GP1_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_GP1_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_GP1_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_GP1_CR1_CEN_BIT);
}

void _timer_gp2_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_GP2_CR1_CEN_BIT));

  volatile uint16_t *timx_psc =
      (volatile uint16_t *)(timer_base + TIM_GP2_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint16_t *timx_cnt =
      (volatile uint16_t *)(timer_base + TIM_GP2_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  auto_reload = (uint16_t)auto_reload;

  volatile uint16_t *timx_arr =
      (volatile uint16_t *)(timer_base + TIM_GP2_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_GP2_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_GP2_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_GP2_CR1_CEN_BIT);
}

void _timer_adv_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_ADV_CR1_CEN_BIT));

  volatile uint32_t *timx_psc =
      (volatile uint32_t *)(timer_base + TIM_ADV_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint32_t *timx_cnt =
      (volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;

  volatile uint32_t *timx_arr =
      (volatile uint32_t *)(timer_base + TIM_ADV_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_ADV_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_ADV_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_ADV_CR1_CEN_BIT);
}
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload) {
  if (timer == TIM1)
    _timer_adv_init(timer, prescaler, auto_reload);
  else if (timer >= TIM2 && timer <= TIM5)
    _timer_gp1_init(timer, prescaler, auto_reload);
  else if (timer >= TIM9 && timer <= TIM11)
    _timer_gp2_init(timer, prescaler, auto_reload);
}

void timer_start(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_ADV_CR1_CEN_BIT);
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_GP1_CR1_CEN_BIT);
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_GP2_CR1_CEN_BIT);
  }
}

void timer_stop(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_ADV_CR1_CEN_BIT));
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_GP1_CR1_CEN_BIT));
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_GP2_CR1_CEN_BIT));
  }
}
void timer_reset(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);

  if (timer == TIM1) {
    *((volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET)) = 0;
  } else if (timer >= TIM2 && timer <= TIM5) {
    *((volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET)) = 0;
  } else if (timer >= TIM9 && timer <= TIM11) {
    *((volatile uint16_t *)(timer_base + TIM_GP2_CNT_OFFSET)) = 0;
  }
}

uint32_t timer_get_count(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    return *((volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET));
  } else if (timer >= TIM2 && timer <= TIM5) {
    return *((volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET));
  } else if (timer >= TIM9 && timer <= TIM11) {
    return *((volatile uint32_t *)(timer_base + TIM_GP2_CNT_OFFSET));
  }
  return 0;
}

void timer_clear_interrupt_flag(hal_timer_t timer) {
  // for tim2-5  & 9only
  uint32_t timer_base = _get_timer_base(timer);
  volatile uint32_t *timer_sr =
      (volatile uint32_t *)(timer_base + TIM_GP1_SR_OFFSET);
  if (timer >= TIM2 && timer <= TIM5) {
    (*timer_sr) &= (~(1 << TIM_GP1_SR_UIF_BIT));
  } else if (timer >= TIM9 && timer <= TIM11)
    (*timer_sr) &= (~(1 << TIM_GP2_SR_UIF_BIT));
}

void TIM2_IRQHandler() {
  timer_clear_interrupt_flag(TIM2);
  hal_handle_interrupt(TIM2_IRQn);
}

void TIM3_IRQHandler() {
  timer_clear_interrupt_flag(TIM3);
  hal_handle_interrupt(TIM3_IRQn);
}

void TIM4_IRQHandler() {
  timer_clear_interrupt_flag(TIM4);
  hal_handle_interrupt(TIM4_IRQn);
}

void TIM5_IRQHandler() {
  timer_clear_interrupt_flag(TIM5);
  hal_handle_interrupt(TIM5_IRQn);
}

void TIM1BRK_TIM9_IRQHandler() {
  timer_clear_interrupt_flag(TIM9);
  hal_handle_interrupt(TIM1_BRK_TIM9_IRQn); // shared with TIM1 BRK
}

void _set_interrupt_enable_bit(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_ADV_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_ADV_DIER_UIE_BIT);
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_GP1_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_GP1_DIER_UIE_BIT);
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_GP2_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_GP2_DIER_UIE_BIT);
  }
}

void timer_enable_interrupt(hal_timer_t timer) {
  // [TODO]: add all timer interrupts
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_enable_interrupt(TIM2_IRQn);
    break;
  case TIM3:
    hal_enable_interrupt(TIM3_IRQn);
    break;
  case TIM4:
    hal_enable_interrupt(TIM4_IRQn);
    break;
  case TIM5:
    hal_enable_interrupt(TIM5_IRQn);
    break;
  case TIM9:
    hal_enable_interrupt(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
  _set_interrupt_enable_bit(timer);
}

void timer_attach_callback(hal_timer_t timer, void (*callback)(void)) {
  // [TODO]: add all timer interrupts
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_attach_callback(TIM2_IRQn, callback);
    break;
  case TIM3:
    hal_interrupt_attach_callback(TIM3_IRQn, callback);
    break;
  case TIM4:
    hal_interrupt_attach_callback(TIM4_IRQn, callback);
    break;
  case TIM5:
    hal_interrupt_attach_callback(TIM5_IRQn, callback);
    break;
  case TIM9:
    hal_interrupt_attach_callback(TIM1_BRK_TIM9_IRQn, callback);
    break;
  default:
    break;
  }
}
void timer_detach_callback(hal_timer_t timer) {
  // [TODO]: add all timer interrupts
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_detach_callback(TIM2_IRQn);
    break;
  case TIM3:
    hal_interrupt_detach_callback(TIM3_IRQn);
    break;
  case TIM4:
    hal_interrupt_detach_callback(TIM4_IRQn);
    break;
  case TIM5:
    hal_interrupt_detach_callback(TIM5_IRQn);
    break;
  case TIM9:
    hal_interrupt_detach_callback(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
}
