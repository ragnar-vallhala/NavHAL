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
 * @file port/cortex-m4/navhal_port_i2c.h
 * @brief Cortex-M4 / STM32F4 I²C port header.
 *
 * @details
 * The public I²C API lives in @c common/hal_i2c.h, which includes this
 * header. This file carries the DMA-backed I²C prototype (available only
 * when the DMA backend is enabled).
 */

#ifndef NAVHAL_PORT_I2C_H
#define NAVHAL_PORT_I2C_H

#include "common/hal_i2c.h"

#include <stdbool.h>
#include "navhal_port_config.h"


#ifdef __cplusplus
extern "C" {
#endif

/* The DMA helpers that used to be declared here now live in common/hal_i2c.h:
 * they are implemented once in src/common/hal_i2c_dma.c, so declaring them per
 * port duplicated the text and kept them outside the conformance gate, which
 * reads include/common only. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_I2C_H */
