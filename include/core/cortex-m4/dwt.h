/**
 * @file core/cortex-m4/dwt.h
 * @brief Cortex-M4 cycle-counter HAL driver interface.
 *
 * @details
 * Standardized cycle-counter API (see `docs/api_standardization.md`). Backed
 * by the Cortex-M4 DWT unit; the public API is named `hal_cycle_counter_*`
 * rather than `dwt_*` so it stays architecture-neutral (a target without a
 * DWT can provide an equivalent counter, or gate the feature off via
 * `NAVHAL_HAS_CYCLE_COUNTER`).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DWT_H
#define CORTEX_M4_DWT_H

#include "common/hal_status.h"
#include <stdint.h>

/**
 * @brief Initialize and start the cycle counter.
 * @return ::HAL_OK.
 */
hal_status_t hal_cycle_counter_init(void);

/**
 * @brief Get the current cycle count.
 * @return 32-bit cycle count.
 */
uint32_t hal_cycle_counter_get(void);

/**
 * @brief Reset the cycle counter to zero.
 * @return ::HAL_OK.
 */
hal_status_t hal_cycle_counter_reset(void);

/**
 * @brief Busy-wait for a number of processor cycles.
 * @param cycles Number of cycles to delay.
 */
void hal_cycle_counter_delay(uint32_t cycles);

/* Deprecated pre-standardization DWT names — removed in M5. */
#include "compat/dwt_compat.h"

#endif // CORTEX_M4_DWT_H
