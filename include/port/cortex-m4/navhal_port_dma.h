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
 * @file port/cortex-m4/navhal_port_dma.h
 * @brief Cortex-M4 / STM32F4 DMA port header.
 *
 * @details
 * The public DMA API lives in @c common/hal_dma.h, which includes this
 * header. This file carries the STM32F4 DMA register map and the
 * deprecated-function-name compat shim. The entire body is compiled only
 * when @c _DMA_ENABLED is defined.
 */

#ifndef NAVHAL_PORT_DMA_H
#define NAVHAL_PORT_DMA_H

#include "common/hal_dma.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DMA_ENABLED

#include "family/dma_reg.h"

/* Deprecated pre-standardization function names — retained as a
 * backward-compat alias behind NAVHAL_DEPRECATED. */
#include "compat/dma_compat.h"

#endif /* _DMA_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_PORT_DMA_H */
