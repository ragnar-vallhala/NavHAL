#ifndef HAL_INTERRUPT_H
#define HAL_INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file hal_interrupt.h
 * @brief Common Interrupt HAL interface for NavHAL.
 *
 * This header defines a common interface for managing hardware interrupts
 * across different microcontroller architectures. It provides functions to
 * enable/disable interrupts, attach/detach callbacks, and set interrupt
 * priorities in an architecture-agnostic manner.
 *
 * Supported architectures include Cortex-M4 (STM32F4 series).
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */
#include "navhal_port_interrupt.h" // Include architecture-specific interrupt definitions
#include "family/interrupt_reg.h" // Include architecture-specific interrupt register definitions

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !HAL_INTERRUPT_H