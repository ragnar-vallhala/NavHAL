/**
 * @file port/avr/navhal_port_dma.h
 * @brief AVR / ATmega328P DMA port header.
 *
 * The ATmega328P has no DMA controller. @c common/hal_dma.h gates its whole
 * body on @c _DMA_ENABLED, which the AVR port never defines, so the DMA API
 * collapses to nothing. This header exists only to satisfy the include.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"

#endif /* NAVHAL_PORT_DMA_H */
