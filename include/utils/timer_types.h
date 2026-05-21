#ifndef TIMER_TYPES_H
#define TIMER_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif
// Timer types available in STM32
typedef enum {
  TIM0,
  TIM1,
  TIM2,
  TIM3,
  TIM4,
  TIM5,
  TIM6,
  TIM7,
  TIM8,
  TIM9,
  TIM10,
  TIM11,
  TIM12,
} hal_timer_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !TIMER_TYPES_H
