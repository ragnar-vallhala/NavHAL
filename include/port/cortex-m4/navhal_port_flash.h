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
 * @file port/cortex-m4/navhal_port_flash.h
 * @brief Cortex-M4 / STM32F4 Flash port header.
 *
 * @details
 * The public Flash API lives in @c common/hal_flash.h, which includes this
 * header. This file carries the deprecated-function-name compat shim.
 */

#ifndef NAVHAL_PORT_FLASH_H
#define NAVHAL_PORT_FLASH_H

#include "common/hal_flash.h"


/* Deprecated pre-standardization function names — retained as a backward-compat alias. */
#include "compat/flash_compat.h"

#endif /* NAVHAL_PORT_FLASH_H */
