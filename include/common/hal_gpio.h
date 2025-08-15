/**
 * @file hal_gpio.h
 * @brief Hardware Abstraction Layer (HAL) interface for General Purpose Input/Output (GPIO).
 *
 * @details
 * This header file provides the interface for including the appropriate
 * GPIO driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/gpio.h` header.
 *
 * The HAL GPIO module provides a platform-independent way to configure
 * and control GPIO pins for input, output, and alternate functions.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#ifdef CORTEX_M4
#include "core/cortex-m4/gpio.h" /**< Include Cortex-M4 specific GPIO driver. */
#endif // CORTEX_M4

#endif // HAL_GPIO_H
