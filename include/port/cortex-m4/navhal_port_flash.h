/**
 * @file port/cortex-m4/navhal_port_flash.h
 * @brief Cortex-M4 / STM32F4 Flash port header.
 *
 * @details
 * The public Flash API lives in @c common/hal_flash.h, which includes this
 * header. This file carries the deprecated-function-name compat shim.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_FLASH_H
#define NAVHAL_PORT_FLASH_H

#include "common/hal_flash.h"


/* Deprecated pre-standardization function names — retained as a backward-compat alias. */
#include "compat/flash_compat.h"

#endif /* NAVHAL_PORT_FLASH_H */
