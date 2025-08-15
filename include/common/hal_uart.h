/**
 * @file hal_uart.h
 * @brief Hardware Abstraction Layer (HAL) interface for UART communication.
 *
 * @details
 * This header file provides the interface for including the appropriate
 * UART driver implementation based on the target architecture.
 * For Cortex-M4 targets, it includes the `core/cortex-m4/uart.h` header.
 *
 * The HAL UART module allows platform-independent serial communication
 * via USART peripherals, supporting functions like transmission and
 * reception of characters and strings.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#ifdef CORTEX_M4
#include "core/cortex-m4/uart.h" /**< Include Cortex-M4 specific UART driver. */
#endif

#endif // !HAL_UART_H
