/**
 * @file interrupt.c
 * @brief Cortex-M4 Interrupt Controller HAL Implementation
 * @details
 * This file provides interrupt management functions for Cortex-M4 based
 * microcontrollers, including:
 * - Enabling/disabling individual interrupts
 * - Attaching/detaching interrupt callbacks
 * - Handling interrupt service routines
 * - Managing up to 128 interrupt sources
 *
 * The implementation uses the NVIC (Nested Vectored Interrupt Controller)
 * and provides a simple callback mechanism for interrupt handling.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/interrupt.h"
#include "common/hal_status.h"
#include <stdint.h>

#define MAX_IRQ 128 ///< Maximum number of supported interrupts

static void (*irq_callbacks[MAX_IRQ])(void) = {0}; ///< Array of interrupt callbacks

/**
 * @brief Enable a specific interrupt
 * @param[in] interrupt Interrupt number to enable
 * @return Status of operation
 * @retval SUCCESS Interrupt enabled successfully
 * @retval FAILURE Invalid interrupt number (Cortex-M internal interrupt)
 *
 * @details
 * Enables the specified interrupt in the NVIC by setting the corresponding
 * bit in the ISER (Interrupt Set Enable Register).
 */
int8_t hal_enable_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ISER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}

/**
 * @brief Disable a specific interrupt
 * @param[in] interrupt Interrupt number to disable
 * @return Status of operation
 * @retval SUCCESS Interrupt disabled successfully
 * @retval FAILURE Invalid interrupt number (Cortex-M internal interrupt)
 *
 * @details
 * Disables the specified interrupt in the NVIC by setting the corresponding
 * bit in the ICER (Interrupt Clear Enable Register).
 */
int8_t hal_disable_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0)
    return FAILURE; // CortexM interrupts

  uint32_t irq_num = (uint32_t)interrupt;
  uint32_t reg_idx = irq_num / 32;
  uint32_t bit_pos = irq_num % 32;

  NVIC->ICER[reg_idx] |= (1 << bit_pos);
  return SUCCESS;
}

/**
 * @brief Attach a callback function to an interrupt
 * @param[in] interrupt Interrupt number to attach to
 * @param[in] callback Function pointer to be called when interrupt occurs
 *
 * @details
 * Registers a callback function for the specified interrupt. The callback
 * will be invoked when the interrupt occurs and is handled by hal_handle_interrupt().
 *
 * @note The callback function should be short and non-blocking
 */
void hal_interrupt_attach_callback(IRQn_Type interrupt,
                                   void (*callback)(void)) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = callback;
}

/**
 * @brief Detach a callback function from an interrupt
 * @param[in] interrupt Interrupt number to detach from
 *
 * @details
 * Removes any previously registered callback for the specified interrupt.
 */
void hal_interrupt_detach_callback(IRQn_Type interrupt) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  irq_callbacks[(uint32_t)interrupt] = 0;
}

/**
 * @brief Handle an interrupt by invoking its registered callback
 * @param[in] interrupt Interrupt number to handle
 *
 * @details
 * This function should be called from the actual interrupt service routine (ISR)
 * to invoke the user-registered callback function. It performs bounds checking
 * and NULL pointer verification before invoking the callback.
 */
void hal_handle_interrupt(IRQn_Type interrupt) {
  if (interrupt < 0 || interrupt >= MAX_IRQ)
    return;
  if (irq_callbacks[(uint32_t)interrupt])
    irq_callbacks[(uint32_t)interrupt]();
}