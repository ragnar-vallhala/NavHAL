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
 * @file port/avr/navhal_port_interrupt.h
 * @brief AVR / ATmega328P interrupt HAL driver interface.
 *
 * @details
 * Standardized interrupt API (see `docs/api_standardization.md`). IRQs are
 * identified by @c hal_irq_t. The ATmega328P has no NVIC: there is no
 * per-vector priority and no per-vector pending/enable register, so the AVR
 * backend (src/arch/avr/) maps this API onto `sei`/`cli`, the per-peripheral
 * enable bits, and a callback table. The @p priority argument is accepted and
 * ignored; per-IRQ enable/pending operations that have no AVR equivalent
 * return ::HAL_ERR_NOT_SUPPORTED.
 */

#ifndef NAVHAL_PORT_INTERRUPT_H
#define NAVHAL_PORT_INTERRUPT_H

#include "common/hal_status.h"
#include "family/interrupt_reg.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback invoked by ::hal_interrupt_dispatch for a registered IRQ. */
typedef void (*hal_interrupt_callback_t)(void);













#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_PORT_INTERRUPT_H */
