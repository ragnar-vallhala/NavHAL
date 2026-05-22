/**
 * @file port/cortex-m4/navhal_port_dma.h
 * @brief Cortex-M4 / STM32F4 DMA port header.
 *
 * @details
 * The public DMA API lives in @c common/hal_dma.h, which includes this
 * header. This file carries the STM32F4 DMA register map and the
 * deprecated-function-name compat shim. The entire body is compiled only
 * when @c _DMA_ENABLED is defined.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DMA_ENABLED

#include "family/dma_reg.h"

/* Deprecated pre-standardization function names — retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED. */
#include "compat/dma_compat.h"

#endif /* _DMA_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_DMA_H */
