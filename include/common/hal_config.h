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
 * Exposes the build-time feature flags (@c NAVHAL_CONFIG_USE_FPU, @c NAVHAL_CONFIG_DRV_DMA,
 * etc.) used by the rest of the HAL. The flags are defined in the Kconfig-
 * generated @c navhal_target.h, which the build force-includes into every
 * translation unit (see the root @c CMakeLists.txt). This header pulls that
 * same generated file in directly when it is reachable on the include path, so
 * a translation unit that includes a HAL header gets the capability flags even
 * if it is not itself force-included — the guard in @c navhal_target.h makes
 * the double include a no-op. A consumer that neither force-includes the
 * generated header nor puts it on the include path still needs the
 * force-include to see the flags.
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

/**
 * @defgroup HAL_CONFIG Config
 * @ingroup HAL_CORE
 * @brief Build-time capability flag entry point.
 * @{
 */

/* Self-source the capability flags from the generated header when it is
 * reachable, so the gate is correct through the include chain and not only via
 * the build's -include. __has_include keeps this harmless where the file is
 * only force-included (absolute path, not on -I): the force-include already
 * defined the macros, and NAVHAL_TARGET_H guards against a second expansion. */
#if defined(__has_include)
#  if __has_include("navhal_target.h")
#    include "navhal_target.h"
#  endif
#endif

#include "navhal_port_config.h"


/** @} */ /* end of group HAL_CONFIG */
#endif /* HAL_CONFIG_H */
