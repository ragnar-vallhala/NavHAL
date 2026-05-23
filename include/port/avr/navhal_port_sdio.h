/**
 * @file port/avr/navhal_port_sdio.h
 * @brief AVR / ATmega328P SDIO port header.
 *
 * The ATmega328P has no SDIO peripheral. @c common/hal_sdio.h gates its whole
 * body on @c _SDIO_ENABLED, which the AVR port never defines, so the SDIO API
 * collapses to nothing. This header exists only to satisfy the include.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_SDIO_H
#define NAVHAL_PORT_SDIO_H

#include "common/hal_sdio.h"

#endif /* NAVHAL_PORT_SDIO_H */
