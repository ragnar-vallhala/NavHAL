/**
 * @file core/cortex-m4/flash.h
 * @brief Cortex-M4 / STM32F4 Flash port header.
 *
 * @details
 * The public Flash API lives in @c common/hal_flash.h, which includes this
 * header. This file carries the deprecated-function-name compat shim.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_FLASH_H
#define CORTEX_M4_FLASH_H

#include "common/hal_flash.h"


/* Deprecated pre-standardization function names — removed in M5. */
#include "compat/flash_compat.h"

#endif /* CORTEX_M4_FLASH_H */
