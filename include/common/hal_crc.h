/**
 * @file hal_crc.h
 * @brief Hardware Abstraction Layer (HAL) interface for CRC calculation.
 *
 * @details
 * This header dispatches to the appropriate architecture-specific CRC driver
 * based on the target macro.  For Cortex-M4 targets it includes
 * @c core/cortex-m4/crc.h, which wraps either the STM32F4 hardware CRC unit
 * (when @c _CRC_HW_ENABLED is defined) or a software fallback.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_CRC_H
#define HAL_CRC_H

#ifdef CORTEX_M4
#include "core/cortex-m4/crc.h" /**< Include Cortex-M4 specific CRC driver. */
#endif                          /* CORTEX_M4 */

#endif /* HAL_CRC_H */
