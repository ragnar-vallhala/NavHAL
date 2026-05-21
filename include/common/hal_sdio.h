/**
 * @file hal_sdio.h
 * @brief Hardware Abstraction Layer (HAL) interface for SDIO.
 *
 * Provides the architecture-agnostic interface for SDIO operations.
 */

#ifndef HAL_SDIO_H
#define HAL_SDIO_H


#ifdef __cplusplus
extern "C" {
#endif
#ifdef CORTEX_M4
#include "core/cortex-m4/sdio.h"
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !HAL_SDIO_H
