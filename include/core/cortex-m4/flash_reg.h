/**
 * @file core/cortex-m4/flash.h
 * @brief Cortex-M4 Flash memory interface definitions.
 *
 * @details
 * This header defines the base address and configuration bit positions
 * for the Flash memory interface registers on Cortex-M4 microcontrollers.
 * These macros are used by the HAL to configure Flash access latency and
 * other Flash-related settings.
 *
 * Macros:
 * - `FLASH_INTERFACE_REGISTER` : Base address of the Flash interface registers.
 * - `FLASH_ACR_LATENCY_BIT`    : Bit position for Flash access latency configuration.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_FLASH_H
#define CORTEX_M4_FLASH_H

#define FLASH_INTERFACE_REGISTER 0x40023C00 /**< Flash Interface base address */
#define FLASH_ACR_LATENCY_BIT 0             /**< Flash ACR Latency bit position */

#endif // !CORTEX_M4_FLASH_H
