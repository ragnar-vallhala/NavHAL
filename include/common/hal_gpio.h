/**
 * @file hal_gpio.h
 * @brief Architecture-agnostic GPIO HAL entry point for NavHAL.
 *
 * This file includes the appropriate architecture-specific GPIO HAL implementation
 * based on the target MCU defined at compile time (e.g., CORTEX_M4).
 *
 * @note This file should be included by user applications via `navhal.h`.
 * Direct inclusion is not recommended unless you are customizing the HAL.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

 #ifndef HAL_GPIO_H
 #define HAL_GPIO_H
 
 /**
  * @defgroup HAL_GPIO GPIO HAL
  * @ingroup NAVHAL
  * @brief GPIO abstraction for multiple architectures.
  * @{
  */
 
 // Cortex-M4 GPIO implementation
 #ifdef CORTEX_M4
 #include "core/cortex-m4/gpio.h"
 #endif // CORTEX_M4
 
 /** @} */ // end of HAL_GPIO
 
 #endif // HAL_GPIO_H
 