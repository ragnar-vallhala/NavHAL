/**
 * @file timer.h
 * @brief Timer and SysTick register defines and API for STM32F4 (Cortex-M4).
 * @details
 * This header contains register offsets, bit definitions, SysTick control,
 * timer base addresses, and the public timer/SysTick API used by the HAL.
 * All defines and function prototypes are intended for the STM32F4 series
 * on a Cortex-M4 core.
 */

#ifndef CORTEX_M4_TIMER_H
#define CORTEX_M4_TIMER_H
#include "common/hal_types.h"
#include "utils/timer_types.h"
#include <stdint.h>

// APB1
#define RCC_APB1ENR_TIM2_OFFSET 0
#define RCC_APB1ENR_TIM3_OFFSET 1
#define RCC_APB1ENR_TIM4_OFFSET 2
#define RCC_APB1ENR_TIM5_OFFSET 3
// APB2
#define RCC_APB2ENR_TIM1_OFFSET 0
#define RCC_APB2ENR_TIM9_OFFSET 16
#define RCC_APB2ENR_TIM10_OFFSET 17
#define RCC_APB2ENR_TIM11_OFFSET 18

// SysTick Control and Status Register
#define SYST_CSR (*(__IO uint32_t *)0xE000E010)
// SysTick Reload Value Register
#define SYST_RVR (*(__IO uint32_t *)0xE000E014)
// SysTick Current Value Register
#define SYST_CVR (*(__IO uint32_t *)0xE000E018)
// SysTick Calibration Register
#define SYST_CALIB (*(__IO uint32_t *)0xE000E01C)
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
void timer_disable_interrupt(
    hal_timer_t timer); // [TODO]Add support for all timers
void timer_clear_interrupt_flag(
    hal_timer_t timer); // [TODO]Add support for all timers
void timer_attach_callback(
    hal_timer_t timer,
    void (*callback)(void)); //[TODO] Add support for all timers
void timer_detach_callback(
    hal_timer_t timer); // [TODO] Add support for all timers

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
uint32_t timer_get_compare(hal_timer_t timer, uint32_t channel);
uint32_t timer_get_arr(hal_timer_t timer, uint32_t channel);
void timer_set_arr(hal_timer_t timer, uint32_t channel, uint32_t arr);
void timer_enable_channel(hal_timer_t timer, uint32_t channel);

void timer_disable_channel(hal_timer_t timer, uint32_t channel);

// Utility Functions
uint32_t timer_get_frequency(hal_timer_t timer);
void timer_set_prescaler(hal_timer_t timer, uint32_t prescaler);
void timer_set_auto_reload(hal_timer_t timer, uint32_t arr);

#endif // !CORTEX_M4_TIMER_H
