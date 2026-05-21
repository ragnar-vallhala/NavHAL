/**
 * @file hal_fpu.h
 * @brief Portable HAL interface for the hardware Floating Point Unit.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_FPU_H
#define HAL_FPU_H

#include "common/hal_status.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable the hardware Floating Point Unit.
 *
 * Enables full access to the CP10/CP11 coprocessors and configures lazy
 * FPU-context stacking.
 *
 * @return ::HAL_OK if the FPU was enabled, or ::HAL_ERR_NOT_SUPPORTED when the
 *         build was configured without hardware-FPU support.
 */
hal_status_t hal_fpu_enable(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "navhal_port_fpu.h"

#endif /* HAL_FPU_H */
