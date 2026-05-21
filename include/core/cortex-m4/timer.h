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


#ifdef __cplusplus
extern "C" {
#endif
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

/* ===== General-purpose timer API ===== */

/** @brief Callback invoked when a timer's update interrupt fires. */
typedef void (*hal_timer_callback_t)(void);

/** @brief Timer base configuration: prescaler (PSC) and auto-reload (ARR). */
typedef struct {
  uint32_t prescaler;   /**< Prescaler (PSC) value. */
  uint32_t auto_reload; /**< Auto-reload (ARR) value. */
} hal_timer_config_t;

/** @brief Initialize a timer with the given prescaler / auto-reload. */
hal_status_t hal_timer_init(hal_timer_t timer, const hal_timer_config_t *cfg);
/** @brief Initialize a timer to a target update frequency in Hz. */
hal_status_t hal_timer_init_freq(hal_timer_t timer, uint32_t freq);
/** @brief Start a timer. */
hal_status_t hal_timer_start(hal_timer_t timer);
/** @brief Stop a timer. */
hal_status_t hal_timer_stop(hal_timer_t timer);
/** @brief Reset a timer's counter to zero. */
hal_status_t hal_timer_reset(hal_timer_t timer);
/** @brief Get a timer's current counter value. */
uint32_t hal_timer_get_count(hal_timer_t timer);

/* Interrupt management */
/** @brief Enable a timer's update interrupt (NVIC + DIER UIE). */
hal_status_t hal_timer_enable_interrupt(hal_timer_t timer);
/** @brief Disable a timer's update interrupt. */
hal_status_t hal_timer_disable_interrupt(hal_timer_t timer);
/** @brief Clear a timer's update interrupt flag. */
hal_status_t hal_timer_clear_interrupt_flag(hal_timer_t timer);
/** @brief Register a timer update-interrupt callback (NULL to clear). */
hal_status_t hal_timer_attach_callback(hal_timer_t timer,
                                       hal_timer_callback_t callback);
/** @brief Remove a timer's update-interrupt callback. */
hal_status_t hal_timer_detach_callback(hal_timer_t timer);

// Timer IRQ Handlers
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);
void TIM9_IRQHandler(void);
void TIM12_IRQHandler(void);

/* Output compare / PWM channels */
/** @brief Set a channel's compare value (configures PWM mode 1). */
hal_status_t hal_timer_set_compare(hal_timer_t timer, uint8_t channel,
                                   uint32_t compare_value);
/** @brief Get a channel's compare value. */
uint32_t hal_timer_get_compare(hal_timer_t timer, uint32_t channel);
/** @brief Enable output on a timer channel (1-4). */
hal_status_t hal_timer_enable_channel(hal_timer_t timer, uint32_t channel);
/** @brief Disable output on a timer channel (1-4). */
hal_status_t hal_timer_disable_channel(hal_timer_t timer, uint32_t channel);

/* Configuration query / update */
/** @brief Get a timer's current base frequency in Hz. */
uint32_t hal_timer_get_frequency(hal_timer_t timer);
/** @brief Set a timer's prescaler (PSC). */
hal_status_t hal_timer_set_prescaler(hal_timer_t timer, uint32_t prescaler);
/** @brief Set a timer's auto-reload (ARR). */
hal_status_t hal_timer_set_auto_reload(hal_timer_t timer,
                                       uint32_t auto_reload);
/** @brief Get a timer's auto-reload (ARR). */
uint32_t hal_timer_get_auto_reload(hal_timer_t timer);

/* Deprecated pre-standardization timebase/timer names — removed in M5. */
#include "compat/timebase_compat.h"
#include "compat/timer_compat.h"


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !CORTEX_M4_TIMER_H
