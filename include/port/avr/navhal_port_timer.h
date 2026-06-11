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
 * @file port/avr/navhal_port_timer.h
 * @brief AVR / ATmega328P timer + timebase port header.
 *
 * The public timer / timebase API lives in @c common/hal_timer.h, which
 * includes this header. ATmega328P timer register access and the
 * timebase-tick ISR live in the driver (via avr-libc `<avr/io.h>` and the
 * `ISR()` macro); no port-specific declarations are needed here.
 */

#ifndef NAVHAL_PORT_TIMER_H
#define NAVHAL_PORT_TIMER_H

#include "common/hal_timer.h"

#endif /* NAVHAL_PORT_TIMER_H */
