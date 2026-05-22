/**
 * @file timer_types.h
 * @brief Timer instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has three timer/counter units: Timer0 (8-bit), Timer1
 * (16-bit) and Timer2 (8-bit, async-capable).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef TIMER_TYPES_H
#define TIMER_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief ATmega328P timer/counter units. */
typedef enum {
  TIM0, /**< Timer0 — 8-bit. */
  TIM1, /**< Timer1 — 16-bit. */
  TIM2, /**< Timer2 — 8-bit, asynchronous-capable. */
} hal_timer_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TIMER_TYPES_H */
