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
 * @file timer_types.h
 * @brief Timer instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has three timer/counter units: Timer0 (8-bit), Timer1
 * (16-bit) and Timer2 (8-bit, async-capable).
 */

#ifndef TIMER_TYPES_H
#define TIMER_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief ATmega328P timer/counter units. */
typedef enum {
  TIM0, /**< Timer0 — 8-bit. */
  TIM1, /**< Timer1 — 16-bit. */
  TIM2, /**< Timer2 — 8-bit, asynchronous-capable. */
} hal_timer_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TIMER_TYPES_H */
