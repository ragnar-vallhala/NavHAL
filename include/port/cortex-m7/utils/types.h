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
 * @file types.h
 * @brief Centralized type definitions include for NavHAL.
 *
 * This header acts as a single include point for all
 * common and peripheral-specific type definitions used
 * throughout the NavHAL project.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef TYPES_H
#define TYPES_H

#include "clock_types.h" /**< Clock-specific type definitions */
#include "gpio_types.h"  /**< GPIO-specific type definitions */
#include "timer_types.h" /**< Timer-specific type definitions */
#include <stdbool.h>     /**< Standard boolean types */
#include <stdint.h>      /**< Standard fixed-width integer types */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TYPES_H
