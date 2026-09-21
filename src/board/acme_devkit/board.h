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
 * @file board.h
 * @brief ACME devkit board description.
 *
 * @details
 * A reference port carries no real hardware, so this names only what a GPIO
 * sample needs. Real boards additionally describe their console UART, clock
 * source and pin map.
 */

#ifndef BOARD_H
#define BOARD_H

#include "common/hal_gpio.h"

/* The board-level names a portable sample expects. A real board also
 * describes its console UART, clock source and header pin map. */
#define LED_BUILTIN GPIO_PA05    /**< User LED, port A pin 5. */
#define LED_ON      HAL_GPIO_HIGH /**< Level that lights ::LED_BUILTIN. */

#endif /* BOARD_H */
