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
 * @file common/hal_reset.c
 * @brief Shared public reset layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_reset_* API. Argument checks
 * live here so every port agrees on them, and the backends keep only the
 * register work.
 */

#include "common/hal_reset.h"
#include "internal/hal_reset_ops.h"

hal_status_t hal_reset_init(void) { return _hal_reset_ops.init(); }

uint32_t hal_reset_get_cause(void) { return _hal_reset_ops.get_cause(); }

hal_status_t hal_system_reset(void) { return _hal_reset_ops.system_reset(); }
