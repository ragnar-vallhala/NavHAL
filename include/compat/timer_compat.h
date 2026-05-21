/**
 * @file timer_compat.h
 * @brief Deprecated pre-standardization general-purpose timer API shim.
 *
 * @details
 * Provides the pre-standardization `timer_*` function names as deprecated
 * inline wrappers over the standardized `hal_timer_*` API. Using a legacy
 * name produces a compiler warning naming the standardized replacement.
 * Included automatically by `port/cortex-m4/navhal_port_timer.h` after the standardized
 * declarations.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_TIMER_COMPAT_H
#define NAVHAL_TIMER_COMPAT_H

#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_timer_init(). */
NAVHAL_DEPRECATED("use hal_timer_init")
static inline void timer_init(hal_timer_t timer, uint32_t prescaler,
                              uint32_t auto_reload) {
  hal_timer_config_t cfg = {.prescaler = prescaler, .auto_reload = auto_reload};
  hal_timer_init(timer, &cfg);
}

/** @deprecated Use hal_timer_init_freq(). */
NAVHAL_DEPRECATED("use hal_timer_init_freq")
static inline void timer_init_freq(hal_timer_t timer, uint32_t freq) {
  hal_timer_init_freq(timer, freq);
}

/** @deprecated Use hal_timer_start(). */
NAVHAL_DEPRECATED("use hal_timer_start")
static inline void timer_start(hal_timer_t timer) { hal_timer_start(timer); }

/** @deprecated Use hal_timer_stop(). */
NAVHAL_DEPRECATED("use hal_timer_stop")
static inline void timer_stop(hal_timer_t timer) { hal_timer_stop(timer); }

/** @deprecated Use hal_timer_reset(). */
NAVHAL_DEPRECATED("use hal_timer_reset")
static inline void timer_reset(hal_timer_t timer) { hal_timer_reset(timer); }

/** @deprecated Use hal_timer_get_count(). */
NAVHAL_DEPRECATED("use hal_timer_get_count")
static inline uint32_t timer_get_count(hal_timer_t timer) {
  return hal_timer_get_count(timer);
}

/** @deprecated Use hal_timer_enable_interrupt(). */
NAVHAL_DEPRECATED("use hal_timer_enable_interrupt")
static inline void timer_enable_interrupt(hal_timer_t timer) {
  hal_timer_enable_interrupt(timer);
}

/** @deprecated Use hal_timer_disable_interrupt(). */
NAVHAL_DEPRECATED("use hal_timer_disable_interrupt")
static inline void timer_disable_interrupt(hal_timer_t timer) {
  hal_timer_disable_interrupt(timer);
}

/** @deprecated Use hal_timer_clear_interrupt_flag(). */
NAVHAL_DEPRECATED("use hal_timer_clear_interrupt_flag")
static inline void timer_clear_interrupt_flag(hal_timer_t timer) {
  hal_timer_clear_interrupt_flag(timer);
}

/** @deprecated Use hal_timer_attach_callback(). */
NAVHAL_DEPRECATED("use hal_timer_attach_callback")
static inline void timer_attach_callback(hal_timer_t timer,
                                         hal_timer_callback_t callback) {
  hal_timer_attach_callback(timer, callback);
}

/** @deprecated Use hal_timer_detach_callback(). */
NAVHAL_DEPRECATED("use hal_timer_detach_callback")
static inline void timer_detach_callback(hal_timer_t timer) {
  hal_timer_detach_callback(timer);
}

/** @deprecated Use hal_timer_set_compare(). */
NAVHAL_DEPRECATED("use hal_timer_set_compare")
static inline void timer_set_compare(hal_timer_t timer, uint8_t channel,
                                     uint32_t compare_value) {
  hal_timer_set_compare(timer, channel, compare_value);
}

/** @deprecated Use hal_timer_get_compare(). */
NAVHAL_DEPRECATED("use hal_timer_get_compare")
static inline uint32_t timer_get_compare(hal_timer_t timer, uint32_t channel) {
  return hal_timer_get_compare(timer, channel);
}

/** @deprecated Use hal_timer_get_auto_reload() (the channel arg is ignored). */
NAVHAL_DEPRECATED("use hal_timer_get_auto_reload")
static inline uint32_t timer_get_arr(hal_timer_t timer, uint32_t channel) {
  (void)channel;
  return hal_timer_get_auto_reload(timer);
}

/** @deprecated Use hal_timer_set_auto_reload() (the channel arg is ignored). */
NAVHAL_DEPRECATED("use hal_timer_set_auto_reload")
static inline void timer_set_arr(hal_timer_t timer, uint32_t channel,
                                 uint32_t arr) {
  (void)channel;
  hal_timer_set_auto_reload(timer, arr);
}

/** @deprecated Use hal_timer_enable_channel(). */
NAVHAL_DEPRECATED("use hal_timer_enable_channel")
static inline void timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  hal_timer_enable_channel(timer, channel);
}

/** @deprecated Use hal_timer_disable_channel(). */
NAVHAL_DEPRECATED("use hal_timer_disable_channel")
static inline void timer_disable_channel(hal_timer_t timer, uint32_t channel) {
  hal_timer_disable_channel(timer, channel);
}

/** @deprecated Use hal_timer_get_frequency(). */
NAVHAL_DEPRECATED("use hal_timer_get_frequency")
static inline uint32_t timer_get_frequency(hal_timer_t timer) {
  return hal_timer_get_frequency(timer);
}

/** @deprecated Use hal_timer_set_prescaler(). */
NAVHAL_DEPRECATED("use hal_timer_set_prescaler")
static inline void timer_set_prescaler(hal_timer_t timer, uint32_t prescaler) {
  hal_timer_set_prescaler(timer, prescaler);
}

/** @deprecated Use hal_timer_set_auto_reload(). */
NAVHAL_DEPRECATED("use hal_timer_set_auto_reload")
static inline void timer_set_auto_reload(hal_timer_t timer, uint32_t arr) {
  hal_timer_set_auto_reload(timer, arr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_TIMER_COMPAT_H */
