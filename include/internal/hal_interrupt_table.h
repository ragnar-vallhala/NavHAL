/*
 * Copyright (C) 2026 NAVRobotec Pvt Ltd
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
 * @file internal/hal_interrupt_table.h
 * @brief The callback table's one entry point for a port's dispatch.
 *
 * @details
 * The table and the attach/detach that fill it are portable -- a bounds check
 * and an indexed call, which every port had written out for itself over its
 * own private array. They live in the common layer now; this is what a port's
 * dispatch calls once it has whatever hardware acknowledgement its controller
 * needs around it.
 *
 * Not public: a caller reaches the table through hal_interrupt_attach_callback.
 */

#ifndef NAVHAL_INTERNAL_HAL_INTERRUPT_TABLE_H
#define NAVHAL_INTERNAL_HAL_INTERRUPT_TABLE_H

#include "common/hal_interrupt.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run the callback registered for @p irq, if there is one.
 *
 * @return true if a callback ran. False means an out-of-range line or an empty
 *         slot, and what to do about that is the port's call: the ARM vector
 *         fallback traps there, with the exception number live in IPSR for a
 *         debugger, rather than returning into a line nobody is servicing.
 */
bool navhal_irq_invoke(hal_irq_t irq);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_INTERNAL_HAL_INTERRUPT_TABLE_H */
