/**
 * @file src/vendor/microchip/pwm/pwm.c
 * @brief ATmega328P PWM HAL driver.
 *
 * @details
 * Implements @c common/hal_pwm.h for the ATmega328P on Timer1 and Timer2
 * (Timer0 is reserved for the timebase). PWM is generated in fast-PWM mode
 * and emitted on the timer's fixed output-compare pins:
 *
 *   Timer1 — channel 1 = OC1A (PB1), channel 2 = OC1B (PB2). 16-bit; the
 *            period is set by ICR1, so any frequency in range is reachable.
 *   Timer2 — channel 1 = OC2A (PB3), channel 2 = OC2B (PD3). 8-bit, TOP
 *            fixed at 0xFF, so the frequency is chosen only by prescaler
 *            and snaps to the nearest achievable value.
 *
 * The output pin is owned by this driver — @ref hal_pwm_init drives the OCnx
 * pin's direction. Channels 3 and 4 do not exist on these timers.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_pwm.h"

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/** @brief TOP value in effect for TIM1 (slot 0) and TIM2 (slot 1). */
static uint16_t s_top[2] = {0xFFFF, 0xFF};

static const uint16_t k_div[] = {1, 8, 64, 256, 1024};
static const uint8_t k_cs[] = {1, 2, 3, 4, 5};

/** @brief State-array index for a PWM-capable timer, or -1. */
static int8_t pwm_index(hal_timer_t timer) {
  if (timer == TIM1)
    return 0;
  if (timer == TIM2)
    return 1;
  return -1; /* TIM0 backs the timebase. */
}

/** @brief Clamp a duty fraction to [0, 1]. */
static float clamp_duty(float d) {
  if (d < 0.0f)
    return 0.0f;
  if (d > 1.0f)
    return 1.0f;
  return d;
}

/** @brief Apply a duty fraction to a channel's compare register. */
static void write_duty(hal_timer_t timer, uint32_t channel, float duty) {
  int8_t i = pwm_index(timer);
  uint32_t cmp = (uint32_t)(clamp_duty(duty) * (float)(s_top[i] + 1u));
  if (timer == TIM1) {
    if (channel == 1)
      OCR1A = (uint16_t)cmp;
    else
      OCR1B = (uint16_t)cmp;
  } else {
    if (channel == 1)
      OCR2A = (uint8_t)cmp;
    else
      OCR2B = (uint8_t)cmp;
  }
}

hal_status_t hal_pwm_init(hal_pwm_handle_t *pwm, uint32_t frequency,
                          float duty_cycle) {
  if (pwm == NULL || frequency == 0u)
    return HAL_ERR_INVALID_ARG;
  int8_t i = pwm_index(pwm->timer);
  if (i < 0 || pwm->channel < 1u || pwm->channel > 2u)
    return HAL_ERR_INVALID_ARG;

  if (pwm->timer == TIM1) {
    /* Fast PWM mode 14: TOP = ICR1, so the frequency is fully programmable. */
    uint8_t cs = 0;
    uint16_t top = 0;
    for (uint8_t k = 0; k < 5; k++) {
      uint32_t period = F_CPU / ((uint32_t)k_div[k] * frequency);
      if (period >= 2u && period <= 65536u) {
        cs = k_cs[k];
        top = (uint16_t)(period - 1u);
        break;
      }
    }
    if (cs == 0)
      return HAL_ERR_INVALID_ARG;
    s_top[0] = top;
    ICR1 = top;
    TCCR1A = (uint8_t)(1u << WGM11); /* mode 14 low bits. */
    TCCR1B = (uint8_t)((1u << WGM13) | (1u << WGM12) | cs);
    /* OC1A = PB1, OC1B = PB2 — drive the chosen channel's pin. */
    DDRB |= (uint8_t)(1u << (pwm->channel == 1u ? PB1 : PB2));
  } else {
    /* Timer2: 8-bit fast PWM mode 3, TOP fixed at 0xFF. */
    uint8_t cs = k_cs[4]; /* default /1024 */
    uint32_t best_err = 0xFFFFFFFFUL;
    static const uint16_t t2_div[] = {1, 8, 32, 64, 128, 256, 1024};
    static const uint8_t t2_cs[] = {1, 2, 3, 4, 5, 6, 7};
    for (uint8_t k = 0; k < 7; k++) {
      uint32_t f = F_CPU / ((uint32_t)t2_div[k] * 256u);
      uint32_t err = (f > frequency) ? (f - frequency) : (frequency - f);
      if (err < best_err) {
        best_err = err;
        cs = t2_cs[k];
      }
    }
    s_top[1] = 0xFF;
    TCCR2A = (uint8_t)((1u << WGM21) | (1u << WGM20)); /* fast PWM mode 3. */
    TCCR2B = cs;
    DDRB |= (uint8_t)(pwm->channel == 1u ? (1u << PB3) : 0u); /* OC2A = PB3 */
    if (pwm->channel == 2u)
      DDRD |= (uint8_t)(1u << PD3); /* OC2B = PD3 */
  }

  write_duty(pwm->timer, pwm->channel, duty_cycle);
  return HAL_OK;
}

hal_status_t hal_pwm_start(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  int8_t i = pwm_index(pwm->timer);
  if (i < 0 || pwm->channel < 1u || pwm->channel > 2u)
    return HAL_ERR_INVALID_ARG;
  /* Connect the OCnx pin: non-inverting mode, COMnx1:0 = 10. */
  if (pwm->timer == TIM1)
    TCCR1A |= (uint8_t)(pwm->channel == 1u ? (1u << COM1A1) : (1u << COM1B1));
  else
    TCCR2A |= (uint8_t)(pwm->channel == 1u ? (1u << COM2A1) : (1u << COM2B1));
  return HAL_OK;
}

hal_status_t hal_pwm_stop(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  int8_t i = pwm_index(pwm->timer);
  if (i < 0 || pwm->channel < 1u || pwm->channel > 2u)
    return HAL_ERR_INVALID_ARG;
  /* Disconnect the OCnx pin (COMnx = 00); it reverts to plain GPIO. */
  if (pwm->timer == TIM1)
    TCCR1A &= (uint8_t)~(pwm->channel == 1u ? (1u << COM1A1) : (1u << COM1B1));
  else
    TCCR2A &= (uint8_t)~(pwm->channel == 1u ? (1u << COM2A1) : (1u << COM2B1));
  return HAL_OK;
}

hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t *pwm, float duty_cycle) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  int8_t i = pwm_index(pwm->timer);
  if (i < 0 || pwm->channel < 1u || pwm->channel > 2u)
    return HAL_ERR_INVALID_ARG;
  write_duty(pwm->timer, pwm->channel, duty_cycle);
  return HAL_OK;
}

hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t *pwm, uint32_t frequency) {
  (void)pwm;
  (void)frequency;
  return HAL_ERR_NOT_SUPPORTED;
}
