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
 * @file port/cortex-m4/navhal_port_clock.h
 * @brief Cortex-M4 clock-control port header.
 *
 * @details
 * The public prototypes live in @c common/hal_clock.h, which includes this
 * header. Retained to preserve the @c navhal_port_clock.h include path.
 */

#ifndef NAVHAL_PORT_CLOCK_H
#define NAVHAL_PORT_CLOCK_H

#include "common/hal_status.h"
#include "utils/clock_types.h"


/**
 * @brief Configure the PLL for a target SYSCLK instead of raw dividers.
 *
 * Picking PLLM/N/P by hand is where a clock config goes quietly wrong: the
 * VCO has a legal range and a wrong N can still produce the right SYSCLK
 * while running the PLL out of spec. This solves for them.
 *
 * @param pll_input ::HAL_CLOCK_SOURCE_HSI or ::HAL_CLOCK_SOURCE_HSE.
 * @param target_hz Desired SYSCLK. Must be exactly reachable.
 * @return ::HAL_ERR_INVALID_ARG if no legal PLL setting hits it.
 */
hal_status_t hal_clock_init_hz(hal_clock_source_t pll_input,
                               uint32_t target_hz);


/* This port carries the deprecated clock names; the shim is included from
 * the common header once the API is declared. */
#define NAVHAL_PORT_CLOCK_COMPAT 1

/* AHB/APB are this port's bus names, so the named accessors are this port's
 * to offer -- but they wrap hal_clock_get_bus_clock, and a static inline
 * cannot call a function nobody has declared. They are defined in
 * common/hal_clock.h behind this marker, after that declaration, so the
 * result does not depend on which header a translation unit opens first.
 */
#define NAVHAL_PORT_CLOCK_BUS_NAMES 1

#endif /* NAVHAL_PORT_CLOCK_H */
