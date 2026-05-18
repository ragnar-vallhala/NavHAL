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
#include "common/hal_status.h"
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

/* ===== Timebase API (SysTick-backed) ===== */

/** @brief Callback invoked on every timebase tick (from the SysTick ISR). */
typedef void (*hal_timebase_callback_t)(void);

/**
 * @brief Initialize the timebase tick at the given period.
 * @param tick_us Tick period in microseconds; must be non-zero.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p tick_us is 0.
 */
hal_status_t hal_timebase_init(uint32_t tick_us);

/** @brief Get the current timebase tick count. */
uint32_t hal_timebase_get_tick(void);

/** @brief Get the configured tick period in microseconds. */
uint32_t hal_timebase_get_tick_duration_us(void);

/** @brief Get the currently configured SysTick reload value (24-bit). */
uint32_t hal_timebase_get_reload_value(void);

/** @brief Get elapsed time since timebase start, in milliseconds. */
uint32_t hal_timebase_get_millis(void);

/** @brief Get elapsed time since timebase start, in microseconds. */
uint32_t hal_timebase_get_micros(void);

/**
 * @brief Register a callback invoked on every timebase tick.
 * @param cb Callback function, or NULL to clear.
 * @return ::HAL_OK.
 */
hal_status_t hal_timebase_set_callback(hal_timebase_callback_t cb);

/** @brief Busy-wait delay for @p ms milliseconds. */
void hal_delay_ms(uint32_t ms);

/** @brief Busy-wait delay for @p us microseconds. */
void hal_delay_us(uint32_t us);

/** @brief SysTick exception handler (vector-table entry). */
void SysTick_Handler(void);

// General Purpose Timer (TIMx) Initialization & Control
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload);
void timer_init_freq(hal_timer_t timer, uint32_t freq);

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

/* Deprecated pre-standardization timebase names — removed in M5. */
#include "compat/timebase_compat.h"

#endif // !CORTEX_M4_TIMER_H
