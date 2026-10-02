/**
 * @file hal_timer_types.h
 * @brief TIMER types, split out so the ops table and the port header can
 *        have them without including the API.
 *
 * common/hal_timer.h ends by including the port header, and everything behind
 * that header -- the family code and the ops table -- needs these types. Taking
 * them from the API header made the include graph circular, which clang-tidy
 * reports at every call site that opens one. Include guards made it compile;
 * they did not make it a graph anyone could follow.
 */
#ifndef HAL_TIMER_TYPES_H
#define HAL_TIMER_TYPES_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "common/hal_types.h"
#include "utils/timer_types.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback invoked on every timebase tick (from the SysTick ISR). */
typedef void (*hal_timebase_callback_t)(void);
/** @brief Callback invoked when a timer's update interrupt fires. */
typedef void (*hal_timer_callback_t)(void);

/** @brief Timer base configuration: prescaler (PSC) and auto-reload (ARR). */
typedef struct {
  uint32_t prescaler;   /**< Prescaler (PSC) value. */
  uint32_t auto_reload; /**< Auto-reload (ARR) value. */
} hal_timer_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_TIMER_TYPES_H */
