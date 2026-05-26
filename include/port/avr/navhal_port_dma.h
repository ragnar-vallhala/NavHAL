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
 * @file port/avr/navhal_port_dma.h
 * @brief AVR / ATmega328P DMA port header.
 *
 * The ATmega328P has no DMA controller. @c common/hal_dma.h gates its whole
 * body on @c _DMA_ENABLED, which the AVR port never defines, so the DMA API
 * collapses to nothing. This header exists only to satisfy the include.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"

#endif /* NAVHAL_PORT_DMA_H */
