/**
 * @file port/avr/navhal_port_timer.h
 * @brief AVR / ATmega328P timer + timebase port header.
 *
 * The public timer / timebase API lives in @c common/hal_timer.h, which
 * includes this header. ATmega328P timer register access and the
 * timebase-tick ISR live in the driver (via avr-libc `<avr/io.h>` and the
 * `ISR()` macro); no port-specific declarations are needed here.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_TIMER_H
#define NAVHAL_PORT_TIMER_H

#include "common/hal_timer.h"

#endif /* NAVHAL_PORT_TIMER_H */
