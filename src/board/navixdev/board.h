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
 * @brief Board-layer aliases for NAVIXDEV - flight controller v0.0.3.
 *
 * @details
 * The development revision that preceded the NAVIXSM-F401RE (v0.1.0). Same
 * STM32F401RE, but it is a breakout board rather than an earlier cut of the
 * same flight controller: sensors sit on pin headers instead of on the PCB,
 * and almost every usable pin is brought out. Firmware for the production
 * board does **not** run here unchanged.
 *
 * What actually differs, pin for pin:
 *
 * | | v0.0.3 (this board) | v0.1.0 |
 * |---|---|---|
 * | IMU | BMX160 module on a 7-pin header, on I2C1 | ICM-42688-P on SPI1 |
 * | IMU interrupts | PC0, PC1 | PC13 |
 * | Barometer | BMP180 module header, on I2C1 | DPS368 on I2C1 |
 * | I2C1 pins | PB8 / PB9 | PB6 / PB7 *and* PB8 / PB9 |
 * | Indicator | RGB LED on PB12/PB13/PB14 | LED on PC0 + WS2812 on PC1 |
 * | Buzzer | PB15 | PA0 |
 * | LoRa | absent | SPI2, PB12..PB15 |
 * | USB | absent | OTG-FS on PA11 / PA12 |
 * | PWM | 12 outputs on three headers | 4 motor outputs |
 *
 * The overlap is the console (USART2), GPS (USART6), micro-SD (SDIO) and the
 * TIM3 motor outputs, which are the same pins on both.
 *
 * Twelve PWM outputs come out on three 4-pin headers, one whole timer each:
 * PWM1 = TIM1 on PA8..PA11, PWM2 = TIM5 on PA0..PA3, PWM3 = TIM3 on
 * PB4/PB5/PB0/PB1. ::BOARD_MOTOR1..4 are PWM3, matching the production
 * board's motor pins. Note PWM2 channels 3 and 4 are PA2/PA3, which are also
 * the console UART -- the two cannot both be in use.
 *
 * ::LED_BUILTIN is one channel of the RGB LED, whose anodes go to the MCU and
 * cathodes to ground, so it is **active high** -- the opposite of the
 * production board. ::LED_ON / ::LED_OFF spell that out, so code written
 * against them ports either way.
 *
 * @warning The RGB LED's anodes are driven straight from the GPIOs with no
 *          series resistors on this revision. Push-pull output at 3V3 is
 *          outside the LED's forward-current rating; drive it briefly, or
 *          configure the pins as open-drain, or PWM them at a low duty.
 *
 * Ten pins are brought to no connector at all -- PA12, PA15, PB2, PB6, PB7,
 * PB10, PC2, PC3, PC4, PC13 -- and are deliberately not aliased here.
 */

#ifndef NAVHAL_BOARD_NAVIXDEV_H
#define NAVHAL_BOARD_NAVIXDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/adc_types.h"
#include "utils/gpio_types.h"
#include "utils/i2c_types.h"
#include "utils/spi_types.h"
#include "utils/timer_types.h"
#include "utils/uart_types.h"

/* On-board indicators — RGB LED D1, common cathode, so active high. */
#define LED_RGB_BLUE  GPIO_PB12
#define LED_RGB_RED   GPIO_PB13
#define LED_RGB_GREEN GPIO_PB14
#define LED_BUILTIN   LED_RGB_GREEN /**< Green channel of the RGB LED. */
#define LED_ON        HAL_GPIO_HIGH
#define LED_OFF       HAL_GPIO_LOW

/* Buzzer — PB15 drives an MMBT2222 through 4.7k; the buzzer itself runs off 5V. */
#define BUZZER GPIO_PB15

/* Board console UART — USART2 (UART1 header, PA2 TX / PA3 RX). */
#define BOARD_CONSOLE_UART     HAL_UART_2
#define BOARD_CONSOLE_UART_IRQ USART2_IRQn

/* GPS — USART6 on PC6 (TX) / PC7 (RX). */
#define BOARD_GPS_UART HAL_UART_6
#define BOARD_GPS_TX   GPIO_PC06
#define BOARD_GPS_RX   GPIO_PC07

/* General-purpose timer. TIM4 rather than the production board's TIM5, which
 * here backs the PWM2 header. */
#define BOARD_GP_TIMER TIM4

/* The board's general PWM output: PWM1 header channel 1. TIM1 is AF1, which is
 * what the portable PWM sample sets, so this is the one trio a sample can drive
 * without touching a motor line. */
#define BOARD_PWM_TIMER   TIM1
#define BOARD_PWM_CHANNEL 1
#define BOARD_PWM_PIN     GPIO_PA08

/* PWM1 header — TIM1 CH1..CH4 (AF1). */
#define BOARD_PWM1_TIMER TIM1
#define BOARD_PWM1_CH1   GPIO_PA08
#define BOARD_PWM1_CH2   GPIO_PA09
#define BOARD_PWM1_CH3   GPIO_PA10
#define BOARD_PWM1_CH4   GPIO_PA11

/* PWM2 header — TIM5 CH1..CH4 (AF2). CH3/CH4 are the console UART pins. */
#define BOARD_PWM2_TIMER TIM5
#define BOARD_PWM2_CH1   GPIO_PA00
#define BOARD_PWM2_CH2   GPIO_PA01
#define BOARD_PWM2_CH3   GPIO_PA02
#define BOARD_PWM2_CH4   GPIO_PA03

/* Motor outputs — PWM3 header: TIM3 CH1..CH4 on PB4/PB5/PB0/PB1 (AF2). */
#define BOARD_MOTOR_TIMER TIM3
#define BOARD_MOTOR1      GPIO_PB04
#define BOARD_MOTOR2      GPIO_PB05
#define BOARD_MOTOR3      GPIO_PB00
#define BOARD_MOTOR4      GPIO_PB01

/* SPI1 header — SCK PA5, MISO PA6, MOSI PA7, NSS PA4. A bare header on this
 * revision: nothing on the board is wired to it. */
#define BOARD_SPI_BUS HAL_SPI_1
#define BOARD_SPI_CS  GPIO_PA04

/* I²C — I2C1 on PB8/PB9 (AF4). Four headers share the bus: two generic, one
 * for a BMP180 barometer module, one for a BMX160 IMU module. */
#define BOARD_I2C_BUS HAL_I2C_1
#define BOARD_I2C_SCL GPIO_PB08
#define BOARD_I2C_SDA GPIO_PB09

/* IMU — BMX160 module on the I²C bus above; both interrupt lines are routed. */
#define BOARD_IMU_INT1 GPIO_PC00
#define BOARD_IMU_INT2 GPIO_PC01

/* micro-SD — SDIO 4-bit: D0..D3 PC8/PC9/PC10/PC11, CK PC12, CMD PD2. */
#define BOARD_SD_CD GPIO_PC05 /**< Card detect. */

/* On-board oscillator frequencies (Hz). A 32.768 kHz crystal (Y1) is fitted on
 * PC14/PC15 for the RTC; the driver assumes that frequency, so it needs no
 * alias here. */
#define BOARD_HSI_FREQ_HZ 16000000U /**< Internal RC, fixed in silicon. */
#define BOARD_HSE_FREQ_HZ  8000000U /**< 8 MHz crystal (Y2) on PH0/PH1. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_BOARD_NAVIXDEV_H */
