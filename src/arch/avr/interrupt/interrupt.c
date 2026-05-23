/**
 * @file src/arch/avr/interrupt/interrupt.c
 * @brief AVR / ATmega328P interrupt HAL backend.
 *
 * @details
 * Implements the @c hal_interrupt_* API (see
 * @c include/port/avr/navhal_port_interrupt.h) for the ATmega328P.
 *
 * The AVR has no NVIC: the vector table is flat, vector order fixes
 * priority, and there is no per-vector enable/pending register — each
 * peripheral owns its own interrupt-enable bit. This backend therefore
 * provides what *is* portable:
 *
 *  - global enable/disable via the SREG I-bit (`sei` / `cli`);
 *  - a callback table that @c hal_interrupt_dispatch routes through, so a
 *    driver ISR can hand an event to a registered ::hal_interrupt_callback_t.
 *
 * Operations with no AVR equivalent (per-IRQ enable/disable/pending) return
 * ::HAL_ERR_NOT_SUPPORTED; priority calls are accepted and ignored.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_interrupt.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>

/** @brief Per-IRQ callback table, indexed by ::hal_irq_t (slot 0 unused). */
static hal_interrupt_callback_t s_callbacks[HAL_IRQ_COUNT];

/** @brief True for an IRQ value that indexes a valid callback slot. */
static inline bool irq_in_range(hal_irq_t irq) {
  return irq >= HAL_IRQ_INT0 && irq < HAL_IRQ_COUNT;
}

/* ---- Per-IRQ control: not a thing on the AVR's flat vector table -------- */

hal_status_t hal_interrupt_enable(hal_irq_t irq) {
  (void)irq;
  return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_interrupt_disable(hal_irq_t irq) {
  (void)irq;
  return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_interrupt_clear_pending(hal_irq_t irq) {
  (void)irq;
  return HAL_ERR_NOT_SUPPORTED;
}

bool hal_interrupt_is_pending(hal_irq_t irq) {
  (void)irq;
  return false;
}

/* ---- Priority: the AVR has none; accept and ignore --------------------- */

hal_status_t hal_interrupt_set_priority(hal_irq_t irq, uint8_t priority) {
  (void)irq;
  (void)priority;
  return HAL_OK;
}

uint8_t hal_interrupt_get_priority(hal_irq_t irq) {
  (void)irq;
  return 0;
}

/* ---- Callback table ---------------------------------------------------- */

hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t callback) {
  if (!irq_in_range(irq))
    return HAL_ERR_INVALID_ARG;
  s_callbacks[irq] = callback;
  return HAL_OK;
}

hal_status_t hal_interrupt_detach_callback(hal_irq_t irq) {
  if (!irq_in_range(irq))
    return HAL_ERR_INVALID_ARG;
  s_callbacks[irq] = NULL;
  return HAL_OK;
}

void hal_interrupt_dispatch(hal_irq_t irq) {
  if (irq_in_range(irq) && s_callbacks[irq] != NULL)
    s_callbacks[irq]();
}

/* ---- Global interrupt enable (SREG I-bit) ------------------------------ */

uint32_t hal_interrupt_disable_global(void) {
  uint8_t saved = SREG;
  cli();
  return saved;
}

void hal_interrupt_enable_global(uint32_t state) {
  /* Restoring the whole SREG restores the I-bit to its prior value. */
  SREG = (uint8_t)state;
}

void hal_interrupt_clear_all_pending(void) {
  /* No NVIC-style "clear all pending" register exists on the AVR. */
}
