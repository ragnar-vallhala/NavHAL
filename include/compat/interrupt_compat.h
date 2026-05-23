/**
 * @file interrupt_compat.h
 * @brief Deprecated pre-standardization interrupt API shim.
 *
 * @details
 * Provides the pre-standardization interrupt function names as deprecated
 * inline wrappers over the standardized `hal_interrupt_*` API. Using a legacy
 * name produces a compiler warning naming the standardized replacement.
 * Included automatically by `port/cortex-m4/navhal_port_interrupt.h`.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_INTERRUPT_COMPAT_H
#define NAVHAL_INTERRUPT_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "family/interrupt_reg.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_interrupt_enable(). */
NAVHAL_DEPRECATED("use hal_interrupt_enable")
static inline hal_status_t hal_enable_interrupt(hal_irq_t irq) {
  return hal_interrupt_enable(irq);
}

/** @deprecated Use hal_interrupt_disable(). */
NAVHAL_DEPRECATED("use hal_interrupt_disable")
static inline hal_status_t hal_disable_interrupt(hal_irq_t irq) {
  return hal_interrupt_disable(irq);
}

/** @deprecated Use hal_interrupt_clear_pending(). */
NAVHAL_DEPRECATED("use hal_interrupt_clear_pending")
static inline hal_status_t hal_clear_interrupt_flag(hal_irq_t irq) {
  return hal_interrupt_clear_pending(irq);
}

/** @deprecated Use hal_interrupt_dispatch(). */
NAVHAL_DEPRECATED("use hal_interrupt_dispatch")
static inline void hal_handle_interrupt(hal_irq_t irq) {
  hal_interrupt_dispatch(irq);
}

/** @deprecated Use hal_interrupt_set_priority(). */
NAVHAL_DEPRECATED("use hal_interrupt_set_priority")
static inline hal_status_t hal_set_interrupt_priority(hal_irq_t irq,
                                                      uint8_t priority) {
  return hal_interrupt_set_priority(irq, priority);
}

/** @deprecated Use hal_interrupt_get_priority(). */
NAVHAL_DEPRECATED("use hal_interrupt_get_priority")
static inline uint8_t hal_get_interrupt_priority(hal_irq_t irq) {
  return hal_interrupt_get_priority(irq);
}

/** @deprecated Use hal_interrupt_is_pending(). */
NAVHAL_DEPRECATED("use hal_interrupt_is_pending")
static inline int hal_is_interrupt_pending(hal_irq_t irq) {
  return hal_interrupt_is_pending(irq) ? 1 : 0;
}

/** @deprecated Use hal_interrupt_enable_global(). */
NAVHAL_DEPRECATED("use hal_interrupt_enable_global")
static inline void hal_enable_global_interrupts(uint32_t state) {
  hal_interrupt_enable_global(state);
}

/** @deprecated Use hal_interrupt_disable_global(). */
NAVHAL_DEPRECATED("use hal_interrupt_disable_global")
static inline uint32_t hal_disable_global_interrupts(void) {
  return hal_interrupt_disable_global();
}

/** @deprecated Use hal_interrupt_clear_all_pending(). */
NAVHAL_DEPRECATED("use hal_interrupt_clear_all_pending")
static inline void hal_clear_all_pending_interrupts(void) {
  hal_interrupt_clear_all_pending();
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_INTERRUPT_COMPAT_H */
