/**
 * @file src/vendor/microchip/timer/timer.c
 * @brief ATmega328P general-purpose timer HAL driver.
 *
 * @details
 * Implements the general-purpose timer API of @c common/hal_timer.h for the
 * ATmega328P, on Timer1 (16-bit) and Timer2 (8-bit). Timer0 is reserved for
 * the timebase driver, so ::TIM0 is rejected here.
 *
 * Each timer runs in CTC mode: the counter resets when it reaches OCRnA, and
 * the compare-match-A interrupt is the periodic "update" event. There is no
 * arbitrary prescaler as on STM32 — the divider is one of a small fixed set,
 * so @ref hal_timer_init snaps @c hal_timer_config_t::prescaler to the
 * nearest supported value.
 *
 * Output-compare / PWM channels are owned by the separate PWM driver
 * (@c hal_pwm.h); the channel entry points here return
 * ::HAL_ERR_NOT_SUPPORTED.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Per-timer state for TIM1 (slot 0) and TIM2 (slot 1). */
typedef struct {
  uint8_t cs;       /**< Clock-select bits used while running (0 = stopped). */
  uint16_t reload;  /**< OCRnA TOP value. */
  uint16_t divider; /**< Prescaler divider for @c cs. */
  hal_timer_callback_t cb;
} timer_state_t;

static timer_state_t s_state[2];

/** @brief Timer1 prescaler table: {divider, CS12:0 bits}. */
static const uint16_t k_t1_div[] = {1, 8, 64, 256, 1024};
static const uint8_t k_t1_cs[] = {1, 2, 3, 4, 5};
/** @brief Timer2 prescaler table: {divider, CS22:0 bits}. */
static const uint16_t k_t2_div[] = {1, 8, 32, 64, 128, 256, 1024};
static const uint8_t k_t2_cs[] = {1, 2, 3, 4, 5, 6, 7};

/** @brief State-array index for a timer, or -1 if not a general timer. */
static int8_t timer_index(hal_timer_t timer) {
  if (timer == TIM1)
    return 0;
  if (timer == TIM2)
    return 1;
  return -1; /* TIM0 is the timebase. */
}

/* ---- prescaler / reload solving --------------------------------------- */

/** @brief Smallest prescaler giving a TOP that fits @p max_top, for @p freq. */
static bool solve(uint32_t freq, uint32_t max_top, const uint16_t *div,
                  const uint8_t *cs, uint8_t n, uint8_t *out_cs,
                  uint16_t *out_top, uint16_t *out_div) {
  for (uint8_t i = 0; i < n; i++) {
    uint32_t count = F_CPU / ((uint32_t)div[i] * freq);
    if (count >= 1 && count <= max_top + 1u) {
      *out_cs = cs[i];
      *out_top = (uint16_t)(count - 1u);
      *out_div = div[i];
      return true;
    }
  }
  return false;
}

/** @brief Prescaler entry whose divider is nearest @p want. */
static void snap(uint32_t want, const uint16_t *div, const uint8_t *cs,
                 uint8_t n, uint8_t *out_cs, uint16_t *out_div) {
  uint8_t best = 0;
  uint32_t best_diff = 0xFFFFFFFFUL;
  for (uint8_t i = 0; i < n; i++) {
    uint32_t diff = (div[i] > want) ? (div[i] - want) : (want - div[i]);
    if (diff < best_diff) {
      best_diff = diff;
      best = i;
    }
  }
  *out_cs = cs[best];
  *out_div = div[best];
}

/* ---- register application --------------------------------------------- */

/** @brief Apply a solved cs / TOP to the hardware and start the timer. */
static void apply(hal_timer_t timer, uint8_t cs, uint16_t top) {
  if (timer == TIM1) {
    TCCR1A = 0;
    TCCR1B = (uint8_t)((1u << WGM12) | cs); /* CTC mode 4; start. */
    OCR1A = top;
    TCNT1 = 0;
  } else { /* TIM2 */
    TCCR2A = (uint8_t)(1u << WGM21); /* CTC mode 2. */
    TCCR2B = cs;                     /* start. */
    OCR2A = (uint8_t)top;
    TCNT2 = 0;
  }
}

hal_status_t hal_timer_init(hal_timer_t timer, const hal_timer_config_t *cfg) {
  int8_t i = timer_index(timer);
  if (i < 0 || cfg == NULL)
    return HAL_ERR_INVALID_ARG;

  uint8_t cs;
  uint16_t divider;
  uint32_t max_top = (timer == TIM1) ? 65535u : 255u;
  if (cfg->auto_reload > max_top)
    return HAL_ERR_INVALID_ARG;

  /* The AVR has no arbitrary prescaler; snap to the nearest divider. */
  if (timer == TIM1)
    snap(cfg->prescaler, k_t1_div, k_t1_cs, 5, &cs, &divider);
  else
    snap(cfg->prescaler, k_t2_div, k_t2_cs, 7, &cs, &divider);

  s_state[i].cs = cs;
  s_state[i].divider = divider;
  s_state[i].reload = (uint16_t)cfg->auto_reload;
  apply(timer, cs, (uint16_t)cfg->auto_reload);
  return HAL_OK;
}

hal_status_t hal_timer_init_freq(hal_timer_t timer, uint32_t freq) {
  int8_t i = timer_index(timer);
  if (i < 0 || freq == 0u)
    return HAL_ERR_INVALID_ARG;

  uint8_t cs;
  uint16_t top, divider;
  bool ok = (timer == TIM1)
                ? solve(freq, 65535u, k_t1_div, k_t1_cs, 5, &cs, &top, &divider)
                : solve(freq, 255u, k_t2_div, k_t2_cs, 7, &cs, &top, &divider);
  if (!ok)
    return HAL_ERR_INVALID_ARG;

  s_state[i].cs = cs;
  s_state[i].divider = divider;
  s_state[i].reload = top;
  apply(timer, cs, top);
  return HAL_OK;
}

hal_status_t hal_timer_start(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  if (timer == TIM1)
    TCCR1B = (uint8_t)((1u << WGM12) | s_state[0].cs);
  else
    TCCR2B = s_state[1].cs;
  return HAL_OK;
}

hal_status_t hal_timer_stop(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  if (timer == TIM1)
    TCCR1B = (uint8_t)(1u << WGM12); /* clock stopped, mode kept. */
  else
    TCCR2B = 0;
  return HAL_OK;
}

hal_status_t hal_timer_reset(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  if (timer == TIM1)
    TCNT1 = 0;
  else
    TCNT2 = 0;
  return HAL_OK;
}

uint32_t hal_timer_get_count(hal_timer_t timer) {
  if (timer == TIM1)
    return TCNT1;
  if (timer == TIM2)
    return TCNT2;
  return 0;
}

hal_status_t hal_timer_enable_interrupt(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  if (timer == TIM1)
    TIMSK1 |= (uint8_t)(1u << OCIE1A);
  else
    TIMSK2 |= (uint8_t)(1u << OCIE2A);
  sei();
  return HAL_OK;
}

hal_status_t hal_timer_disable_interrupt(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  if (timer == TIM1)
    TIMSK1 &= (uint8_t)~(1u << OCIE1A);
  else
    TIMSK2 &= (uint8_t)~(1u << OCIE2A);
  return HAL_OK;
}

hal_status_t hal_timer_clear_interrupt_flag(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  /* The compare-match flag is cleared by writing 1 to it. */
  if (timer == TIM1)
    TIFR1 = (uint8_t)(1u << OCF1A);
  else
    TIFR2 = (uint8_t)(1u << OCF2A);
  return HAL_OK;
}

hal_status_t hal_timer_attach_callback(hal_timer_t timer,
                                       hal_timer_callback_t callback) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  s_state[i].cb = callback;
  return HAL_OK;
}

hal_status_t hal_timer_detach_callback(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  s_state[i].cb = NULL;
  return HAL_OK;
}

uint32_t hal_timer_get_frequency(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  if (i < 0 || s_state[i].divider == 0u)
    return 0;
  return F_CPU / ((uint32_t)s_state[i].divider * (s_state[i].reload + 1u));
}

hal_status_t hal_timer_set_prescaler(hal_timer_t timer, uint32_t prescaler) {
  int8_t i = timer_index(timer);
  if (i < 0)
    return HAL_ERR_INVALID_ARG;
  uint8_t cs;
  uint16_t divider;
  if (timer == TIM1)
    snap(prescaler, k_t1_div, k_t1_cs, 5, &cs, &divider);
  else
    snap(prescaler, k_t2_div, k_t2_cs, 7, &cs, &divider);
  s_state[i].cs = cs;
  s_state[i].divider = divider;
  return hal_timer_start(timer); /* re-apply the clock select. */
}

hal_status_t hal_timer_set_auto_reload(hal_timer_t timer,
                                       uint32_t auto_reload) {
  int8_t i = timer_index(timer);
  uint32_t max_top = (timer == TIM1) ? 65535u : 255u;
  if (i < 0 || auto_reload > max_top)
    return HAL_ERR_INVALID_ARG;
  s_state[i].reload = (uint16_t)auto_reload;
  if (timer == TIM1)
    OCR1A = (uint16_t)auto_reload;
  else
    OCR2A = (uint8_t)auto_reload;
  return HAL_OK;
}

uint32_t hal_timer_get_auto_reload(hal_timer_t timer) {
  int8_t i = timer_index(timer);
  return (i < 0) ? 0 : s_state[i].reload;
}

/* ---- Output-compare / PWM channels: handled by the PWM driver --------- */

hal_status_t hal_timer_set_compare(hal_timer_t timer, uint8_t channel,
                                   uint32_t compare_value) {
  (void)timer;
  (void)channel;
  (void)compare_value;
  return HAL_ERR_NOT_SUPPORTED; /* use hal_pwm_* */
}

uint32_t hal_timer_get_compare(hal_timer_t timer, uint32_t channel) {
  (void)timer;
  (void)channel;
  return 0;
}

hal_status_t hal_timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  (void)timer;
  (void)channel;
  return HAL_ERR_NOT_SUPPORTED; /* use hal_pwm_* */
}

hal_status_t hal_timer_disable_channel(hal_timer_t timer, uint32_t channel) {
  (void)timer;
  (void)channel;
  return HAL_ERR_NOT_SUPPORTED; /* use hal_pwm_* */
}

/* ---- update interrupts ------------------------------------------------ */

ISR(TIMER1_COMPA_vect) {
  if (s_state[0].cb != NULL)
    s_state[0].cb();
}

ISR(TIMER2_COMPA_vect) {
  if (s_state[1].cb != NULL)
    s_state[1].cb();
}
