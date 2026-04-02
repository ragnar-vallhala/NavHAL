/**
 * @file dwt.h
 * @brief Cortex-M4 DWT (Data Watchpoint and Trace) API.
 *
 * @details
 * This header provides the architecture-specific API for configuring
 * and using the DWT unit for cycle counting and high-resolution timing.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DWT_H
#define CORTEX_M4_DWT_H

#include <stdint.h>

/**
 * @brief Initialize the DWT unit.
 *
 * @details
 * Enables the trace unit (TRCENA in DEMCR) and starts the cycle counter
 * (CYCCNTENA in DWT_CTRL).
 */
void dwt_init(void);

/**
 * @brief Get the current cycle count.
 *
 * @return 32-bit cycle count (CYCCNT).
 */
uint32_t dwt_get_cycles(void);

/**
 * @brief Reset the cycle counter to zero.
 */
void dwt_reset_cycles(void);

/**
 * @brief Block for a specified number of processor cycles.
 *
 * @param cycles Number of cycles to delay.
 */
void dwt_delay_cycles(uint32_t cycles);

#endif // CORTEX_M4_DWT_H
