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
 * @file port/x86_64/navhal_port_config.h
 * @brief Bare-metal x86-64 build-time feature flags.
 *
 * @details
 * Counterpart of the Cortex-M4 and AVR headers. Every port needs one, because
 * common/hal_config.h includes it unconditionally — without it, any translation
 * unit that reaches hal_config.h fails to compile on this port, which is how its
 * absence was found.
 *
 * The PC port exposes none of the optional peripherals the HAL gates on, so this
 * defines no flags and the gated API headers collapse to nothing, exactly as on
 * AVR.
 */

#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H

/* Intentionally empty: the x86-64 port exposes no gated optional peripheral. */

#endif /* NAVHAL_PORT_CONFIG_H */
