/**
 * @file gpio_compat.h
 * @brief Deprecated pre-standardization GPIO API shim.
 *
 * @details
 * Provides the pre-standardization GPIO function names as thin, inline
 * wrappers over the standardized `hal_gpio_*` API. Each wrapper is marked
 * deprecated, so using a legacy name produces a compiler warning that names
 * the standardized replacement. Existing drivers and samples keep building
 * during the M2-M5 migration; this header is included automatically by
 * `port/cortex-m4/navhal_port_gpio.h`.
 *
 * This header — and every symbol it defines — is retained as a backward-compat layer behind NAVHAL_DEPRECATED. New code MUST
 * use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_GPIO_COMPAT_H
#define NAVHAL_GPIO_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/gpio_types.h"


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_gpio_set_mode(). */
NAVHAL_DEPRECATED("use hal_gpio_set_mode")
static inline hal_status_t hal_gpio_setmode(hal_gpio_pin_t pin,
                                            hal_gpio_mode_t mode,
                                            hal_gpio_pull_t pull) {
  return hal_gpio_set_mode(pin, mode, pull);
}

/** @deprecated Use hal_gpio_get_mode(). */
NAVHAL_DEPRECATED("use hal_gpio_get_mode")
static inline hal_gpio_mode_t hal_gpio_getmode(hal_gpio_pin_t pin) {
  return hal_gpio_get_mode(pin);
}

/** @deprecated Use hal_gpio_write(). */
NAVHAL_DEPRECATED("use hal_gpio_write")
static inline void hal_gpio_digitalwrite(hal_gpio_pin_t pin,
                                         hal_gpio_state_t state) {
  hal_gpio_write(pin, state);
}

/** @deprecated Use hal_gpio_read(). */
NAVHAL_DEPRECATED("use hal_gpio_read")
static inline hal_gpio_state_t hal_gpio_digitalread(hal_gpio_pin_t pin) {
  return hal_gpio_read(pin);
}

/** @deprecated Use hal_gpio_enable_clock(). */
NAVHAL_DEPRECATED("use hal_gpio_enable_clock")
static inline hal_status_t hal_gpio_enable_rcc(hal_gpio_pin_t pin) {
  return hal_gpio_enable_clock(pin);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_GPIO_COMPAT_H */
