/**
 * @file port/avr/navhal_port_fpu.h
 * @brief AVR / ATmega328P FPU port header.
 *
 * The ATmega328P has no hardware FPU (::NAVHAL_HAS_FPU is 0). @c hal_fpu_enable
 * returns ::HAL_ERR_NOT_SUPPORTED on this target. No port-specific
 * declarations here.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_FPU_H
#define NAVHAL_PORT_FPU_H

#include "common/hal_fpu.h"

#endif /* NAVHAL_PORT_FPU_H */
