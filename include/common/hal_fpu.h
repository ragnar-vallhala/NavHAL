/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file hal_fpu.h
 * @brief Portable HAL interface for the hardware Floating Point Unit.
 */

#ifndef HAL_FPU_H
#define HAL_FPU_H

/**
 * @defgroup HAL_FPU Fpu
 * @ingroup HAL_DRIVERS
 * @brief Hardware Floating-Point Unit enable.
 * @{
 */

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


/** @} */ /* end of group HAL_FPU */
#endif /* HAL_FPU_H */
