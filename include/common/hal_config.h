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
 * @file hal_config.h
 * @brief Portable HAL configuration-macro entry point.
 *
 * @details
 * Exposes the build-time feature flags (@c _FPU_ENABLED, @c _DMA_ENABLED,
 * etc.) used by the rest of the HAL. The actual macro definitions live in
 * the per-port @c config.h for now; they will be relocated to a Kconfig-
 * generated @c navhal_target.h in WI4.3.
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

/**
 * @defgroup HAL_CONFIG Config
 * @ingroup HAL_CORE
 * @brief Build-time capability flag entry point.
 * @{
 */

#include "navhal_port_config.h"


/** @} */ /* end of group HAL_CONFIG */
#endif /* HAL_CONFIG_H */
