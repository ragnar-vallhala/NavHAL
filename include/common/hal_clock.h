/**
 * @file hal_clock.h
 * @brief Hardware Abstraction Layer (HAL) interface for clock management.
 *
 * @details
 * This header file provides the interface for including the appropriate
 * clock driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/clock.h` header.
 *
 * The HAL clock module provides an abstraction over the hardware-specific
 * clock control mechanisms, enabling platform-independent code to manage
 * system and peripheral clocks.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#ifdef CORTEX_M4
#include "core/cortex-m4/clock.h" /**< Include Cortex-M4 specific clock driver. */
#endif // CORTEX_M4

#endif // HAL_CLOCK_H
