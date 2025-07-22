/**
 * @file navhal.h
 * @brief Root header file for the NavHAL project by NavRobotec.
 *
 * NavHAL is the official hardware abstraction layer (HAL) developed by NavRobotec.
 * It provides a clean, modular, and extensible C interface to abstract peripheral
 * control (such as GPIO, UART, timers) across multiple microcontroller architectures.
 *
 * Include this file in your application to access the full HAL API.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef NAVHAL_H
#define NAVHAL_H

/**
 * @defgroup NAVHAL Core HAL
 * @brief Top-level includes for the NavHAL framework.
 * @{
 */

/// Common GPIO HAL header (architecture-agnostic interface)
#include "common/hal_gpio.h"

/// Common Clock HAL header (architecture-agnostic interface)
#include "common/hal_clock.h"

/// Common UART HAL header (architecture-agnostic interface)
#include "common/hal_uart.h"


/** @} */ // end of NAVHAL

#endif // NAVHAL_H
