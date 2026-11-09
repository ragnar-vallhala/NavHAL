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
 * @file port/avr/navhal_port_clock.h
 * @brief AVR / ATmega328P clock port header.
 *
 * The public clock API lives in @c common/hal_clock.h, which includes this
 * header. The ATmega328P port adds no port-specific declarations here.
 */

#ifndef NAVHAL_PORT_CLOCK_H
#define NAVHAL_PORT_CLOCK_H

#include "common/hal_status.h"
#include "utils/clock_types.h"


/* This port carries the deprecated clock names; the shim is included from
 * the common header once the API is declared. */
#define NAVHAL_PORT_CLOCK_COMPAT 1

#endif /* NAVHAL_PORT_CLOCK_H */
