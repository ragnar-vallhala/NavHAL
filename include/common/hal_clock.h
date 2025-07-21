/**
 * @file hal_clock.h
 * @brief Architecture-agnostic Clock HAL entry point for NavHAL.
 *
 * This file includes the appropriate architecture-specific Clock HAL implementation
 * based on the target MCU defined at compile time (e.g., CORTEX_M4).
 *
 * @note This file should be included by user applications via `navhal.h`.
 * Direct inclusion is not recommended unless you are customizing the HAL.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-21
 */

 #ifndef HAL_CLOCK_H
 #define HAL_CLOCK_H
 
 /**
  * @defgroup HAL_CLOCK Clock HAL
  * @ingroup NAVHAL
  * @brief Clock abstraction for multiple architectures.
  * @{
  */
 
 // Cortex-M4 Clock implementation
 #ifdef CORTEX_M4
 #include "core/cortex-m4/clock.h"
 #endif // CORTEX_M4
 
 /** @} */ // end of HAL_CLOCK
 
 #endif // HAL_CLOCK_H
 