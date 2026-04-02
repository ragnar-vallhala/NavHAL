/**
 * @file hal_dwt.h
 * @brief Hardware Abstraction Layer (HAL) interface for DWT (Data Watchpoint
 * and Trace).
 *
 * @details
 * This header file provides the interface for including the appropriate
 * DWT driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/dwt.h` header.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_DWT_H
#define HAL_DWT_H

#ifdef CORTEX_M4
#include "core/cortex-m4/dwt.h" /**< Include Cortex-M4 specific DWT driver. */
#endif                          // CORTEX_M4

#endif // HAL_DWT_H
