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
 * @file port/avr/navhal_port_gpio.h
 * @brief AVR / ATmega328P GPIO port header.
 *
 * @details
 * The portable GPIO prototypes (init, set_mode, get_mode, …) live in
 * @c common/hal_gpio.h, which includes this header. The hot-path pin
 * accessors — write / read / toggle — are defined here as @c static
 * @c inline: for a compile-time-constant pin (the common case) the
 * pin-to-register decode constant-folds away and avr-gcc emits a single
 * `sbi` / `cbi` (or a one-byte store for the PINx-write toggle). This gives
 * the AVR port the same zero-cost GPIO the Cortex-M4 port has — a runtime
 * pin variable still works, it just costs the decode.
 */

#ifndef NAVHAL_PORT_GPIO_H
#define NAVHAL_PORT_GPIO_H

#include "common/hal_gpio.h"

#include <avr/io.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a logic level to a pin (hot path).
 *
 * Folds to a single `sbi` / `cbi` when @p pin and @p state are constant.
 */
/* The inlined hot path is the vendor's: GPIO is silicon, not CPU core.
 * Two vendors on one arch share nothing here, so the accessors live with the
 * register map they are written against. */
#include "family/gpio_inline.h"

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_PORT_GPIO_H */
