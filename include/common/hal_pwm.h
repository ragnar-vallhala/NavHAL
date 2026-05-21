/**
 * @file hal_pwm.h
 * @brief Hardware Abstraction Layer (HAL) interface for Pulse Width Modulation (PWM).
 *
 * @details
 * This header file provides the interface for including the appropriate
 * PWM driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/pwm.h` header.
 *
 * The HAL PWM module offers a platform-independent way to configure and
 * control PWM signals for tasks such as motor control, signal generation,
 * and brightness control.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_PWM_H
#define HAL_PWM_H


#ifdef __cplusplus
extern "C" {
#endif
#ifdef CORTEX_M4
#include "core/cortex-m4/pwm.h" /**< Include Cortex-M4 specific PWM driver. */
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !HAL_PWM_H
