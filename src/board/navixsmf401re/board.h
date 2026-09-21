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
 * @brief Board-layer aliases for NAVIXSM-F401RE v0.1.0.
 *
 * @details
 * Custom STM32F401RE flight-controller board: IMU on SPI1, LoRa on SPI2,
 * GPS on USART6, iBus receiver on USART2, micro-SD on SDIO, USB OTG-FS
 * device on PA11/PA12, four motor outputs on TIM3.
 *
 * ::LED_BUILTIN is wired 3V3 -> 1k -> LED -> PC0, so it is **active low**:
 * drive the pin LOW to light it. ::LED_ON / ::LED_OFF spell that out.
 */

#ifndef NAVHAL_BOARD_NAVIXSM_F401RE_H
#define NAVHAL_BOARD_NAVIXSM_F401RE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/adc_types.h"
#include "utils/gpio_types.h"
#include "utils/i2c_types.h"
#include "utils/spi_types.h"
#include "utils/timer_types.h"
#include "utils/uart_types.h"

/* On-board indicators */
#define LED_BUILTIN GPIO_PC00 /**< Blue LED, active low (anode to 3V3). */
#define LED_ON      HAL_GPIO_LOW
#define LED_OFF     HAL_GPIO_HIGH
#define LED_RGB     GPIO_PC01 /**< RGB LED data line. */
#define BUZZER      GPIO_PA00

/* Board console UART — USART2 (iBus header, PA2 TX / PA3 RX). */
#define BOARD_CONSOLE_UART     HAL_UART_2
#define BOARD_CONSOLE_UART_IRQ USART2_IRQn

/* GPS — USART6 on PC6 (TX) / PC7 (RX). */
#define BOARD_GPS_UART HAL_UART_6
#define BOARD_GPS_TX   GPIO_PC06
#define BOARD_GPS_RX   GPIO_PC07

/* General-purpose timer. */
#define BOARD_GP_TIMER TIM5

/* Motor outputs — TIM3 CH1..CH4 on PB4/PB5/PB0/PB1 (AF2). */
#define BOARD_PWM_TIMER TIM3
#define BOARD_MOTOR1    GPIO_PB04
#define BOARD_MOTOR2    GPIO_PB05
#define BOARD_MOTOR3    GPIO_PB00
#define BOARD_MOTOR4    GPIO_PB01

/* IMU — SPI1 on PA5/PA6/PA7, CS on PA4, INT1 on PC13. */
#define BOARD_SPI_BUS  HAL_SPI_1
#define BOARD_SPI_CS   GPIO_PA04
#define BOARD_IMU_INT1 GPIO_PC13

/* LoRa — SPI2 on PB13/PB14/PB15, NSS PB12, RST PB2, BUSY PA8, DIO1 PA15. */
#define BOARD_LORA_SPI  HAL_SPI_2
#define BOARD_LORA_NSS  GPIO_PB12
#define BOARD_LORA_RST  GPIO_PB02
#define BOARD_LORA_BUSY GPIO_PA08
#define BOARD_LORA_DIO1 GPIO_PA15

/* I²C — both headers are I2C1 pin options on the F401 (AF4). */
#define BOARD_I2C_BUS HAL_I2C_1
#define BOARD_I2C_SCL GPIO_PB06
#define BOARD_I2C_SDA GPIO_PB07
#define BOARD_I2C_EXT_SCL GPIO_PB08
#define BOARD_I2C_EXT_SDA GPIO_PB09

/* micro-SD — SDIO 4-bit: D0..D3 PC8/PC9/PC10/PC11, CK PC12, CMD PD2. */
#define BOARD_SD_CD GPIO_PC05 /**< Card detect. */

/* USB OTG-FS device — PA11 (DM) / PA12 (DP), VBUS sense on PA9. */
#define BOARD_USB_DM   GPIO_PA11
#define BOARD_USB_DP   GPIO_PA12
#define BOARD_USB_VBUS GPIO_PA09

/* On-board oscillator frequencies (Hz) */
#define BOARD_HSI_FREQ_HZ 16000000U /**< Internal RC, fixed in silicon. */
#define BOARD_HSE_FREQ_HZ  8000000U /**< 8 MHz crystal on PH0/PH1. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_BOARD_NAVIXSM_F401RE_H */
