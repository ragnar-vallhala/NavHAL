/**
 * @file hal_timer.h
 * @brief Hardware Abstraction Layer (HAL) interface for timer peripherals.
 *
 * @details
 * This header file provides the interface for including the appropriate
 * timer driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/timer.h` header.
 *
 * The HAL timer module allows platform-independent configuration and
 * control of hardware timers for tasks such as delays, periodic interrupts,
 * and event scheduling.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#ifdef CORTEX_M4
#include "core/cortex-m4/timer.h" /**< Include Cortex-M4 specific timer driver. */
#endif

#endif // !HAL_TIMER_H
