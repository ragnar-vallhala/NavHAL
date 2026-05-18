/**
 * @file core/cortex-m4/fpu.h
 * @brief Cortex-M4 hardware FPU HAL driver interface.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_FPU_H
#define CORTEX_M4_FPU_H

#include "common/hal_status.h"

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

#endif // CORTEX_M4_FPU_H
