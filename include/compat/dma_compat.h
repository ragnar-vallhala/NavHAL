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
 * @file dma_compat.h
 * @brief Deprecated pre-standardization DMA API shim.
 *
 * @details
 * Maps the pre-standardization `dma_*` function names onto the standardized
 * `hal_dma_*` API as deprecated inline wrappers. Included automatically by
 * `port/cortex-m4/navhal_port_dma.h` (inside its `_DMA_ENABLED` guard).
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 */

#ifndef NAVHAL_DMA_COMPAT_H
#define NAVHAL_DMA_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_dma_init(). */
NAVHAL_DEPRECATED("use hal_dma_init")
static inline hal_status_t dma_init(const hal_dma_config_t *cfg) {
  return hal_dma_init(cfg);
}

/** @deprecated Use hal_dma_start(). */
NAVHAL_DEPRECATED("use hal_dma_start")
static inline hal_status_t dma_start(const hal_dma_config_t *cfg) {
  return hal_dma_start(cfg);
}

/** @deprecated Use hal_dma_stop(). */
NAVHAL_DEPRECATED("use hal_dma_stop")
static inline hal_status_t dma_stop(const hal_dma_config_t *cfg) {
  return hal_dma_stop(cfg);
}

/** @deprecated Use hal_dma_transfer_complete(). */
NAVHAL_DEPRECATED("use hal_dma_transfer_complete")
static inline int dma_transfer_complete(const hal_dma_config_t *cfg) {
  return hal_dma_transfer_complete(cfg) ? 1 : 0;
}

/** @deprecated Use hal_dma_clear_flags(). */
NAVHAL_DEPRECATED("use hal_dma_clear_flags")
static inline hal_status_t dma_clear_flags(const hal_dma_config_t *cfg) {
  return hal_dma_clear_flags(cfg);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_DMA_COMPAT_H */
