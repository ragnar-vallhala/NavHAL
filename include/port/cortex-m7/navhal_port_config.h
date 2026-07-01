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
 * @file port/cortex-m7/navhal_port_config.h
 * @brief Deprecated no-op — retained only for include compatibility.
 *
 * @details
 * This header used to bridge the Kconfig-generated @c NAVHAL_HAS_* capability
 * macros into legacy @c _*_ENABLED guard flags. That bridge is gone: the build
 * now **force-includes** @c navhal_target.h into every translation unit, so the
 * single macro family @c NAVHAL_CONFIG_DRV_* (and the deprecated
 * @c NAVHAL_HAS_* aliases) is ambient everywhere. Drivers gate directly on
 * @c "#if NAVHAL_CONFIG_DRV_<X>". Nothing needs to be defined here.
 */

#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H

/* Intentionally empty: gating macros are force-included via navhal_target.h. */

#endif /* NAVHAL_PORT_CONFIG_H */
