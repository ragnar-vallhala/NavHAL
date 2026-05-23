/**
 * @file port/avr/navhal_port_dwt.h
 * @brief AVR / ATmega328P cycle-counter port header.
 *
 * The ATmega328P has no DWT-style cycle counter (::NAVHAL_HAS_CYCLE_COUNTER
 * is 0). @c common/hal_dwt.h still declares the @c hal_cycle_counter_* API;
 * calling it on this target is a link error by design. No port-specific
 * declarations here.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_DWT_H
#define NAVHAL_PORT_DWT_H

#include "common/hal_dwt.h"

#endif /* NAVHAL_PORT_DWT_H */
