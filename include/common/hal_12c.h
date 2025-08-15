/**
 * @file hal_12c.h
 * @brief Hardware Abstraction Layer (HAL) interface for I2C communication.
 *
 * @details
 * This header file provides the interface for including the appropriate
 * I2C driver implementation based on the target architecture. 
 * For Cortex-M4 targets, it includes the `core/cortex-m4/i2c.h` header.
 *
 * The HAL layer allows higher-level code to remain platform-independent
 * by abstracting hardware-specific implementations.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_I2C_H
#define HAL_I2C_H

#ifdef CORTEX_M4
#include "core/cortex-m4/i2c.h" /**< Include Cortex-M4 specific I2C driver. */
#endif

#endif // !HAL_I2C_H
