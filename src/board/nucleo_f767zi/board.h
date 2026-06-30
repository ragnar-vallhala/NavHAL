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
 * @brief Board-layer aliases for the ST Nucleo-F767ZI (Nucleo-144).
 *
 * @details
 * Resolves the portable, board-agnostic identifiers used by application code
 * (`LED_BUILTIN`, `USER_BUTTON`, `D0`..`D15`, `A0`..`A5`) into the per-MCU
 * core GPIO enum (`GPIO_PB00`, ...) defined for STM32F767ZI. The same names
 * exist on every board that ships a `board.h`, so a sample written against
 * `LED_BUILTIN` can move between boards with only a board switch.
 *
 * Also exposes the on-board oscillator frequencies — HSE comes from the
 * ST-LINK MCO on the Nucleo, not a discrete crystal.
 *
 * Reference: UM1974 (Nucleo-144 user manual), §6 (hardware layout) and the
 * F767ZI Arduino/Zio connector tables.
 */

#ifndef NAVHAL_BOARD_NUCLEO_F767ZI_H
#define NAVHAL_BOARD_NUCLEO_F767ZI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/gpio_types.h"
#include "utils/i2c_types.h"
#include "utils/spi_types.h"
#include "utils/timer_types.h"
#include "utils/uart_types.h"

/* On-board indicators / inputs */
#define LED_BUILTIN  GPIO_PB00  /**< LD1 (green user LED). */
#define LD1_GREEN    GPIO_PB00  /**< LD1 green. */
#define LD2_BLUE     GPIO_PB07  /**< LD2 blue. */
#define LD3_RED      GPIO_PB14  /**< LD3 red. */
#define USER_BUTTON  GPIO_PC13  /**< B1 user button (active-high on this board). */

/* Board console UART — USART3 is wired to the ST-LINK virtual COM port
 * (TX = PD8, RX = PD9, AF7). NOTE: the F7 USART IP differs from the F4 model
 * the shared uart.c targets; UART is disabled in this board's defconfig until
 * the F7 driver path lands (see docs/stm32f767zi_port_plan.md, F7-2). */
#define BOARD_CONSOLE_UART      HAL_UART_3
#define BOARD_CONSOLE_UART_IRQ  USART3_IRQn
#define BOARD_CONSOLE_UART_TX   GPIO_PD08
#define BOARD_CONSOLE_UART_RX   GPIO_PD09

/* General-purpose timer. */
#define BOARD_GP_TIMER  TIM2

/* PWM output — TIM3 channel 1 on PB4 (alternate function AF2). */
#define BOARD_PWM_TIMER    TIM3
#define BOARD_PWM_CHANNEL  1
#define BOARD_PWM_PIN      GPIO_PB04

/* I2C bus — I2C1 on PB8 (SCL) / PB9 (SDA). */
#define BOARD_I2C_BUS  HAL_I2C_1
#define BOARD_I2C_SCL  GPIO_PB08
#define BOARD_I2C_SDA  GPIO_PB09

/* SPI bus — SPI1; CS on PD14. */
#define BOARD_SPI_BUS  HAL_SPI_1
#define BOARD_SPI_CS   GPIO_PD14

/* Arduino-compatible digital headers (CN7..CN10 on the Nucleo-144) */
#define D0   GPIO_PG09  /**< USART6 RX on the Arduino header. */
#define D1   GPIO_PG14  /**< USART6 TX on the Arduino header. */
#define D2   GPIO_PF15
#define D3   GPIO_PE13
#define D4   GPIO_PF14
#define D5   GPIO_PE11
#define D6   GPIO_PE09
#define D7   GPIO_PF13
#define D8   GPIO_PF12
#define D9   GPIO_PD15
#define D10  GPIO_PD14  /**< SPI1 CS (default). */
#define D11  GPIO_PA07  /**< SPI1 MOSI. */
#define D12  GPIO_PA06  /**< SPI1 MISO. */
#define D13  GPIO_PA05  /**< SPI1 SCK. */
#define D14  GPIO_PB09  /**< I2C1 SDA. */
#define D15  GPIO_PB08  /**< I2C1 SCL. */

/* Arduino-compatible analog headers (CN9) */
#define A0   GPIO_PA03
#define A1   GPIO_PC00
#define A2   GPIO_PC03
#define A3   GPIO_PF03
#define A4   GPIO_PF05
#define A5   GPIO_PF10

/* On-board oscillator frequencies (Hz) */
#define BOARD_HSI_FREQ_HZ  16000000U  /**< Internal RC, fixed in silicon. */
#define BOARD_HSE_FREQ_HZ   8000000U  /**< MCO from the ST-LINK MCU. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_BOARD_NUCLEO_F767ZI_H */
