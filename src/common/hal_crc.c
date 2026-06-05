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
 * @file src/common/hal_crc.c
 * @brief Shared public CRC layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_crc_* API. It hoists the
 * NULL-config check (previously duplicated in every vendor's @c crc.c) and
 * forwards to the active backend's ::_hal_crc_ops. The compute/accumulate
 * functions pass through unchanged — their "return the current accumulator on
 * NULL/empty input" behaviour is backend state, so it stays in the backend.
 */

#include "common/hal_crc.h"
#include "internal/hal_crc_ops.h"

#include <stddef.h>

hal_status_t hal_crc_init(const hal_crc_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_crc_ops.init(cfg);
}

uint32_t hal_crc_compute(const uint8_t *data, uint32_t len) {
  return _hal_crc_ops.compute(data, len);
}

uint32_t hal_crc_accumulate(const uint8_t *data, uint32_t len) {
  return _hal_crc_ops.accumulate(data, len);
}

hal_status_t hal_crc_reset(void) {
  return _hal_crc_ops.reset();
}
