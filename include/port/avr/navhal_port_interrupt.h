/**
 * @file port/avr/navhal_port_interrupt.h
 * @brief AVR / ATmega328P interrupt HAL driver interface.
 *
 * @details
 * Standardized interrupt API (see `docs/api_standardization.md`). IRQs are
 * identified by ::hal_irq_t. The ATmega328P has no NVIC: there is no
 * per-vector priority and no per-vector pending/enable register, so the AVR
 * backend (src/arch/avr/) maps this API onto `sei`/`cli`, the per-peripheral
 * enable bits, and a callback table. The @p priority argument is accepted and
 * ignored; per-IRQ enable/pending operations that have no AVR equivalent
 * return ::HAL_ERR_NOT_SUPPORTED.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_INTERRUPT_H
#define NAVHAL_PORT_INTERRUPT_H

#include "common/hal_status.h"
#include "family/interrupt_reg.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback invoked by ::hal_interrupt_dispatch for a registered IRQ. */
typedef void (*hal_interrupt_callback_t)(void);

/**
 * @brief Enable a specific interrupt source.
 * @param irq IRQ identifier.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG for an unknown IRQ, or
 *         ::HAL_ERR_NOT_SUPPORTED where the source has no generic enable.
 */
hal_status_t hal_interrupt_enable(hal_irq_t irq);

/**
 * @brief Disable a specific interrupt source.
 * @param irq IRQ identifier.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG, or ::HAL_ERR_NOT_SUPPORTED.
 */
hal_status_t hal_interrupt_disable(hal_irq_t irq);

/**
 * @brief Clear the pending flag of a specific interrupt.
 * @param irq IRQ identifier.
 * @return ::HAL_OK, ::HAL_ERR_INVALID_ARG, or ::HAL_ERR_NOT_SUPPORTED.
 */
hal_status_t hal_interrupt_clear_pending(hal_irq_t irq);

/**
 * @brief Set the priority of an interrupt.
 * @param irq      IRQ identifier.
 * @param priority Priority value — ignored on the ATmega328P (no NVIC).
 * @return ::HAL_OK.
 */
hal_status_t hal_interrupt_set_priority(hal_irq_t irq, uint8_t priority);

/**
 * @brief Get the priority of an interrupt.
 * @param irq IRQ identifier.
 * @return Always 0 on the ATmega328P — priorities are fixed by vector order.
 */
uint8_t hal_interrupt_get_priority(hal_irq_t irq);

/**
 * @brief Check whether a specific interrupt is pending.
 * @param irq IRQ identifier.
 * @return true if pending, false otherwise.
 */
bool hal_interrupt_is_pending(hal_irq_t irq);

/**
 * @brief Register a callback to be invoked for a specific IRQ.
 * @param irq      IRQ identifier.
 * @param callback Callback function, or NULL to clear.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t callback);

/**
 * @brief Remove the callback registered for a specific IRQ.
 * @param irq IRQ identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_detach_callback(hal_irq_t irq);

/**
 * @brief Invoke the callback registered for an IRQ (called from an ISR).
 * @param irq IRQ identifier that occurred.
 */
void hal_interrupt_dispatch(hal_irq_t irq);

/**
 * @brief Restore the global interrupt-enable state (SREG I-bit).
 * @param state State previously returned by ::hal_interrupt_disable_global.
 */
void hal_interrupt_enable_global(uint32_t state);

/**
 * @brief Disable all maskable interrupts globally (`cli`).
 * @return The previous global interrupt state, for use with
 *         ::hal_interrupt_enable_global.
 */
uint32_t hal_interrupt_disable_global(void);

/**
 * @brief Clear every pending interrupt flag the backend can reach.
 */
void hal_interrupt_clear_all_pending(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_PORT_INTERRUPT_H */
