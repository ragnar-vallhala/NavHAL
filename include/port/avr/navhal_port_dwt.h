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
 * @file port/avr/navhal_port_dwt.h
 * @brief AVR / ATmega328P cycle-counter port header.
 *
 * The ATmega328P has no DWT-style cycle counter (@c NAVHAL_HAS_CYCLE_COUNTER
 * is 0). @c common/hal_dwt.h still declares the @c hal_cycle_counter_* API;
 * calling it on this target is a link error by design. No port-specific
 * declarations here.
 */

#ifndef NAVHAL_PORT_DWT_H
#define NAVHAL_PORT_DWT_H

#include "common/hal_dwt.h"

#endif /* NAVHAL_PORT_DWT_H */
