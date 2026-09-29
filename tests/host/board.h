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
 * @file tests/host/board.h
 * @brief Board description for the host driver suite.
 *
 * @details
 * The deep SIL suite compiles the real STM32F7 drivers against a simulated MMIO
 * backing store, without running Kconfig -- so it has no generated board.h and
 * must supply its own. This is that file: a notional board, not a real one.
 *
 * Hand-written on purpose, and not a contradiction of board.h being generated
 * everywhere else. A real board is described by its Kconfig because a consumer
 * must be able to change it; this is a test fixture, and what it describes is
 * whatever the suite wants the drivers to see.
 *
 * The values match what the F7 drivers used to hardcode, so the suite exercises
 * the same pins it always did. A driver reaching for a board fact this file does
 * not define fails to compile here, which is the intended outcome: the fixture
 * should be extended deliberately, not silently defaulted.
 */

#ifndef NAVHAL_TEST_HOST_BOARD_H
#define NAVHAL_TEST_HOST_BOARD_H

#include "utils/gpio_types.h"
#include "utils/uart_types.h"

/* USARTs, as the F7 driver addressed them before the pins came from the board. */
#define BOARD_USART1_TX HAL_GPIO_PIN(HAL_GPIO_PORT_B, 6)
#define BOARD_USART1_RX HAL_GPIO_PIN(HAL_GPIO_PORT_B, 7)
#define BOARD_USART1_AF 7

#define BOARD_USART2_TX HAL_GPIO_PIN(HAL_GPIO_PORT_A, 2)
#define BOARD_USART2_RX HAL_GPIO_PIN(HAL_GPIO_PORT_A, 3)
#define BOARD_USART2_AF 7

/* The Nucleo-F767ZI's ST-LINK virtual COM port sits here. */
#define BOARD_USART3_TX HAL_GPIO_PIN(HAL_GPIO_PORT_D, 8)
#define BOARD_USART3_RX HAL_GPIO_PIN(HAL_GPIO_PORT_D, 9)
#define BOARD_USART3_AF 7

#define BOARD_USART6_TX HAL_GPIO_PIN(HAL_GPIO_PORT_C, 6)
#define BOARD_USART6_RX HAL_GPIO_PIN(HAL_GPIO_PORT_C, 7)
#define BOARD_USART6_AF 8

/* SPIs, as the F7 driver addressed them before the pins came from the board. */
#define BOARD_SPI1_SCK  HAL_GPIO_PIN(HAL_GPIO_PORT_A, 5)
#define BOARD_SPI1_MISO HAL_GPIO_PIN(HAL_GPIO_PORT_A, 6)
#define BOARD_SPI1_MOSI HAL_GPIO_PIN(HAL_GPIO_PORT_A, 7)
#define BOARD_SPI1_AF   5

#define BOARD_SPI2_SCK  HAL_GPIO_PIN(HAL_GPIO_PORT_B, 13)
#define BOARD_SPI2_MISO HAL_GPIO_PIN(HAL_GPIO_PORT_B, 14)
#define BOARD_SPI2_MOSI HAL_GPIO_PIN(HAL_GPIO_PORT_B, 15)
#define BOARD_SPI2_AF   5

#endif /* NAVHAL_TEST_HOST_BOARD_H */
