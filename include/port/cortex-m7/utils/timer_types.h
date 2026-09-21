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

#ifndef TIMER_TYPES_H
#define TIMER_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif
// Timer types available in STM32
typedef enum {
  TIM0,
  TIM1,
  TIM2,
  TIM3,
  TIM4,
  TIM5,
  TIM6,
  TIM7,
  TIM8,
  TIM9,
  TIM10,
  TIM11,
  TIM12,
} hal_timer_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !TIMER_TYPES_H
