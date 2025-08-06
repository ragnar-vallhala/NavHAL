#ifndef CORTEX_M4_TIMER_H
#define CORTEX_M4_TIMER_H
#include "utils/types.h"

#define RCC_BASE 0x40023800

#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB1ENR_TIM2_OFFSET 0
#define RCC_APB1ENR_TIM3_OFFSET 1
#define RCC_APB1ENR_TIM4_OFFSET 2
#define RCC_APB1ENR_TIM5_OFFSET 3

#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_APB2ENR_TIM1_OFFSET 0
#define RCC_APB2ENR_TIM9_OFFSET 16
#define RCC_APB2ENR_TIM10_OFFSET 17
#define RCC_APB2ENR_TIM11_OFFSET 18

#define TIM1_BASE 0x40010000
#define TIM2_BASE 0x40000000
#define TIM3_BASE 0x40000400
#define TIM4_BASE 0x40000800
#define TIM5_BASE 0x40000C00
#define TIM9_BASE 0x40014000
#define TIM10_BASE 0x40014400
#define TIM11_BASE 0x40014800

// Advance Timer 1
// Labled as ADV
#define TIM_ADV_CR1_OFFSET 0x00
#define TIM_ADV_CR1_CEN_BIT 0x00
#define TIM_ADV_PSC_OFFSET                                                     \
  0x28 // Timer tick = ABP1_CLK/(prescaler+1), Use only lower 16 bits, upper
       // bits are reserved
#define TIM_ADV_ARR_OFFSET                                                     \
  0x2C // Upper 16 bits are only available for TIM2 and TIM5, reserved for rest
#define TIM_ADV_EGR_OFFSET 0x14 // Event generator
#define TIM_ADV_EGR_UG_BIT 0x00 // Reinitialize the timer
#define TIM_ADV_CNT_OFFSET 0X24

// GP Timer 2-5 on APB1
// Labled as GP1
#define TIM_GP1_CR1_OFFSET 0x00
#define TIM_GP1_CR1_CEN_BIT 0x00
#define TIM_GP1_PSC_OFFSET                                                     \
  0x28 // Timer tick = ABP1_CLK/(prescaler+1), Use only lower 16 bits, upper
       // bits are reserved
#define TIM_GP1_ARR_OFFSET                                                     \
  0x2C // Upper 16 bits are only available for TIM2 and TIM5, reserved for rest
#define TIM_GP1_EGR_OFFSET 0x14 // Event generator
#define TIM_GP1_EGR_UG_BIT 0x00 // Reinitialize the timer
#define TIM_GP1_CNT_OFFSET 0X24

// GP Timer 9-11 on APB1
// Labled as GP2
#define TIM_GP2_CR1_OFFSET 0x00
#define TIM_GP2_CR1_CEN_BIT 0x00
#define TIM_GP2_PSC_OFFSET                                                     \
  0x28 // Timer tick = ABP2_CLK/(prescaler+1), Use only lower 16 bits, upper
       // bits are reserved
#define TIM_GP2_ARR_OFFSET 0x2C // only lower 16 bits availabnle
#define TIM_GP2_EGR_OFFSET 0x14 // Event generator
#define TIM_GP2_EGR_UG_BIT 0x0  // Reinitialize the timer
#define TIM_GP2_CNT_OFFSET 0X24

// SysTick Control and Status Register
#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
// SysTick Reload Value Register
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
// SysTick Current Value Register
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)
// SysTick Calibration Register
#define SYST_CALIB (*(volatile uint32_t *)0xE000E01C)
#define SYST_CSR_EN_BIT 0
#define SYST_CSR_TICKINT_BIT 1
#define SYST_CSR_CLKSOURCE_BIT 2
// SysTick Timer Functions
// TIM5 is used for systick
void systick_init(uint32_t tick_us);
void delay_ms(uint32_t ms);
void delay_us(uint64_t us);
uint64_t hal_get_tick(void);
uint32_t hal_get_tick_duration_us(void);
uint32_t hal_get_tick_reload_value(void);
uint32_t hal_get_millis(void);
uint32_t hal_get_micros(void);
void SysTick_Handler(void); // ISR for systick interrupts
void hal_systick_set_callback(void (*cb)(void));

// General Purpose Timer (TIMx) Initialization & Control
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload);
void timer_start(hal_timer_t timer);
void timer_stop(hal_timer_t timer);
void timer_reset(hal_timer_t timer);
uint32_t timer_get_count(hal_timer_t timer);

// Timer Interrupt Management
void timer_enable_interrupt(hal_timer_t timer);
void timer_disable_interrupt(hal_timer_t timer);
void timer_clear_interrupt_flag(hal_timer_t timer);
void timer_attach_callback(hal_timer_t timer, void (*callback)(void));
void timer_detach_callback(hal_timer_t timer);

// Timer IRQ Handlers
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);
void TIM9_IRQHandler(void);
void TIM12_IRQHandler(void);

// PWM and Output Compare (Future Stage)
void timer_set_compare(hal_timer_t timer, uint8_t channel,
                       uint32_t compare_value);
void timer_enable_pwm(hal_timer_t timer, uint8_t channel);
void timer_disable_pwm(hal_timer_t timer, uint8_t channel);

// Utility Functions
uint32_t timer_get_frequency(hal_timer_t timer);
void timer_set_prescaler(hal_timer_t timer, uint32_t prescaler);
void timer_set_auto_reload(hal_timer_t timer, uint32_t arr);

#endif // !CORTEX_M4_TIMER_H
