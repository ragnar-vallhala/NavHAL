/**
 * @file interrupt.h
 * @brief NVIC register definitions, IRQ numbers, and HAL interrupt control API for STM32F4.
 * @details
 * This header provides:
 * - NVIC register mapping for the Cortex-M4 interrupt controller.
 * - Enumeration of all Cortex-M4 and STM32F4-specific IRQ numbers.
 * - HAL functions to enable/disable interrupts, set priorities, attach/detach callbacks,
 *   and query pending/active interrupt status.
 *
 * The definitions are tailored for the STM32F4 series but are based on the ARM Cortex-M4
 * NVIC architecture. All register mappings match the reference manual.
 */

#ifndef INTERRUPT_H
#define INTERRUPT_H
#ifndef CORTEX_M4_INTERRUPT_H
#define CORTEX_M4_INTERRUPT_H

#include "core/cortex-m4/interrupt_reg.h"
#include <stdint.h>
int8_t
hal_enable_interrupt(IRQn_Type interrupt); // return 0 if enable success else -1
int8_t hal_disable_interrupt(
    IRQn_Type interrupt); // return 0 if enable success else -1

void hal_clear_interrupt_flag(
    IRQn_Type interrupt); // [TODO] add later specific to the port
void hal_interrupt_attach_callback(IRQn_Type interrupt, void (*callback)(void));

/**
 * @brief Detach a callback function from an interrupt.
 * @param interrupt IRQ number.
 */
void hal_interrupt_detach_callback(IRQn_Type interrupt);

/**
 * @brief Central interrupt handler function.
 * @param interrupt IRQ number that occurred.
 */
void hal_handle_interrupt(IRQn_Type interrupt);

/**
 * @brief Set priority for a specific interrupt.
 * @param interrupt IRQ number.
 * @param priority Priority value.
 * @note [TODO] Implementation pending.
 */
void hal_set_interrupt_priority(IRQn_Type interrupt, uint8_t priority);

/**
 * @brief Get priority for a specific interrupt.
 * @param interrupt IRQ number.
 * @return Priority value.
 * @note [TODO] Implementation pending.
 */
uint8_t hal_get_interrupt_priority(IRQn_Type interrupt);

/**
 * @brief Check if an interrupt is pending.
 * @param interrupt IRQ number.
 * @return 1 if pending, 0 if not.
 * @note [TODO] Implementation pending.
 */
int hal_is_interrupt_pending(IRQn_Type interrupt);

/**
 * @brief Enable global interrupts.
 * @param state Optional state flag.
 * @note [TODO] Implementation pending.
 */
void hal_enable_global_interrupts(uint32_t state);

/**
 * @brief Disable global interrupts.
 * @return Previous interrupt state.
 * @note [TODO] Implementation pending.
 */
uint32_t hal_disable_global_interrupts(void);

/**
 * @brief Clear all pending interrupts.
 * @note [TODO] Implementation pending.
 */
void hal_clear_all_pending_interrupts(void);

#endif //! CORTEX_M4_INTERRUPT_H
