/**
 * @file src/vendor/microchip/timebase/timebase.c
 * @brief ATmega328P timebase HAL driver.
 *
 * @details
 * Implements the timebase API of @c common/hal_timer.h for the ATmega328P.
 * Timer0 runs in CTC mode and its compare-match-A interrupt increments a
 * 32-bit tick counter. Timer0 is reserved for the timebase; the general-
 * purpose timer driver uses Timer1 / Timer2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <util/delay.h>

static volatile uint32_t s_ticks = 0;       /**< Elapsed timebase ticks. */
static uint32_t s_tick_us = 0;              /**< Tick period, microseconds. */
static uint8_t s_reload = 0;                /**< OCR0A compare value. */
static hal_timebase_callback_t s_callback = NULL;

/* Timer0 prescaler options: divider and the matching CS0[2:0] bit pattern. */
struct presc {
  uint16_t div;
  uint8_t cs;
};
static const struct presc k_presc[] = {
    {1, 1}, {8, 2}, {64, 3}, {256, 4}, {1024, 5}};

hal_status_t hal_timebase_init(uint32_t tick_us) {
  if (tick_us == 0)
    return HAL_ERR_INVALID_ARG;

  /* Timer0 counts at F_CPU/div; the period is (OCR0A+1) counts. */
  uint32_t total = (F_CPU / 1000000UL) * tick_us;
  uint8_t cs = 0;
  uint16_t count = 0;
  for (uint8_t i = 0; i < (uint8_t)(sizeof(k_presc) / sizeof(k_presc[0]));
       i++) {
    uint32_t c = total / k_presc[i].div;
    if (c >= 1 && c <= 256) {
      cs = k_presc[i].cs;
      count = (uint16_t)c;
      break;
    }
  }
  if (cs == 0) /* tick_us out of the representable range for Timer0. */
    return HAL_ERR_INVALID_ARG;

  s_tick_us = tick_us;
  s_ticks = 0;
  s_reload = (uint8_t)(count - 1);

  TCCR0A = (uint8_t)(1u << WGM01); /* CTC mode, OCR0A = TOP. */
  TCCR0B = cs;                     /* clock source / prescaler. */
  OCR0A = s_reload;
  TCNT0 = 0;
  TIMSK0 = (uint8_t)(1u << OCIE0A); /* enable compare-match-A interrupt. */
  sei();                            /* timebase needs global interrupts on. */
  return HAL_OK;
}

ISR(TIMER0_COMPA_vect) {
  s_ticks++;
  if (s_callback != NULL)
    s_callback();
}

uint32_t hal_timebase_get_tick(void) {
  uint32_t t;
  uint8_t sreg = SREG;
  cli(); /* 32-bit read is non-atomic on an 8-bit core. */
  t = s_ticks;
  SREG = sreg;
  return t;
}

uint32_t hal_timebase_get_tick_duration_us(void) { return s_tick_us; }

uint32_t hal_timebase_get_reload_value(void) { return s_reload; }

uint32_t hal_timebase_get_millis(void) {
  return (hal_timebase_get_tick() * s_tick_us) / 1000UL;
}

uint32_t hal_timebase_get_micros(void) {
  return hal_timebase_get_tick() * s_tick_us;
}

hal_status_t hal_timebase_set_callback(hal_timebase_callback_t cb) {
  s_callback = cb; /* NULL clears the callback. */
  return HAL_OK;
}

void hal_delay_ms(uint32_t ms) {
  while (ms--)
    _delay_ms(1);
}

void hal_delay_us(uint32_t us) {
  while (us--)
    _delay_us(1);
}
