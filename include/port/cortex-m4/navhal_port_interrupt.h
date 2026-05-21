/**
 * @file port/cortex-m4/navhal_port_interrupt.h
 * @brief Cortex-M4 / STM32F4 interrupt-controller (NVIC) HAL driver interface.
 *
 * @details
 * Standardized interrupt API (see `docs/api_standardization.md`). All public
 * functions use the `hal_interrupt_` prefix and `snake_case` verbs. Per-IRQ
 * control operations return ::hal_status_t; queries return their value
 * directly. IRQs are identified by ::IRQn_Type.
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
/**
 * @brief Callback invoked by ::hal_interrupt_dispatch for a registered IRQ.
 */
typedef void (*hal_interrupt_callback_t)(void);

/**
 * @brief Enable a specific interrupt in the NVIC.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_enable(IRQn_Type irq);

/**
 * @brief Disable a specific interrupt in the NVIC.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_disable(IRQn_Type irq);

/**
 * @brief Clear the pending flag of a specific interrupt.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a non-NVIC (negative) IRQ.
 */
hal_status_t hal_interrupt_clear_pending(IRQn_Type irq);

/**
 * @brief Set the priority of a specific interrupt or system exception.
 * @param irq      IRQ number.
 * @param priority Priority value (lower is more urgent; normalized to the
 *                 implemented priority bits).
 * @return ::HAL_OK.
 */
hal_status_t hal_interrupt_set_priority(IRQn_Type irq, uint8_t priority);

/**
 * @brief Get the priority of a specific interrupt or system exception.
 * @param irq IRQ number.
 * @return Normalized priority value.
 */
uint8_t hal_interrupt_get_priority(IRQn_Type irq);

/**
 * @brief Check whether a specific interrupt is pending.
 * @param irq IRQ number.
 * @return true if pending, false otherwise.
 */
bool hal_interrupt_is_pending(IRQn_Type irq);

/**
 * @brief Register a callback to be invoked for a specific IRQ.
 * @param irq      IRQ number.
 * @param callback Callback function, or NULL to clear.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_attach_callback(IRQn_Type irq,
                                           hal_interrupt_callback_t callback);

/**
 * @brief Remove the callback registered for a specific IRQ.
 * @param irq IRQ number.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an out-of-range IRQ.
 */
hal_status_t hal_interrupt_detach_callback(IRQn_Type irq);

/**
 * @brief Invoke the callback registered for an IRQ (called from an ISR).
 * @param irq IRQ number that occurred.
 */
void hal_interrupt_dispatch(IRQn_Type irq);

/**
 * @brief Restore the global interrupt-enable state (PRIMASK).
 * @param state State previously returned by ::hal_interrupt_disable_global.
 */
void hal_interrupt_enable_global(uint32_t state);

/**
 * @brief Disable all maskable interrupts globally.
 * @return The previous global interrupt state, for use with
 *         ::hal_interrupt_enable_global.
 */
uint32_t hal_interrupt_disable_global(void);

/**
 * @brief Clear the pending flag of every NVIC interrupt.
 */
void hal_interrupt_clear_all_pending(void);

/* Deprecated pre-standardization interrupt names — removed in M5. */
#include "compat/interrupt_compat.h"


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_PORT_INTERRUPT_H
