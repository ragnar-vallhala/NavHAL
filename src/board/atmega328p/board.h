/**
 * @file board.h
 * @brief Board-layer aliases for the ATmega328P (Arduino Uno pinout).
 *
 * @details
 * Resolves the portable, board-agnostic identifiers used by application code
 * (`LED_BUILTIN`, `D0`..`D13`, `A0`..`A5`, `BOARD_CONSOLE_UART`) into the
 * per-MCU core enums for the ATmega328P. The same alias names exist on every
 * board that ships a `board.h`, so a sample written against `LED_BUILTIN`
 * builds unchanged on the Nucleo-F401RE and on this board.
 *
 * Pin assignments follow the Arduino Uno, whose MCU is the ATmega328P.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_BOARD_ATMEGA328P_H
#define NAVHAL_BOARD_ATMEGA328P_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/gpio_types.h"
#include "utils/i2c_types.h"
#include "utils/spi_types.h"
#include "utils/timer_types.h"
#include "utils/uart_types.h"

/* On-board indicator / input */
#define LED_BUILTIN  GPIO_PB05  /**< On-board LED, shared with D13 (SCK). */
#define USER_BUTTON  GPIO_PD02  /**< D2 — wire a button to GND; uses a pull-up. */

/* Arduino-Uno digital header D0..D13 */
#define D0   GPIO_PD00  /**< USART0 RXD. */
#define D1   GPIO_PD01  /**< USART0 TXD. */
#define D2   GPIO_PD02
#define D3   GPIO_PD03
#define D4   GPIO_PD04
#define D5   GPIO_PD05
#define D6   GPIO_PD06
#define D7   GPIO_PD07
#define D8   GPIO_PB00
#define D9   GPIO_PB01
#define D10  GPIO_PB02  /**< SPI SS. */
#define D11  GPIO_PB03  /**< SPI MOSI. */
#define D12  GPIO_PB04  /**< SPI MISO. */
#define D13  GPIO_PB05  /**< SPI SCK; also the on-board LED. */

/* Arduino-Uno analog header A0..A5 */
#define A0   GPIO_PC00
#define A1   GPIO_PC01
#define A2   GPIO_PC02
#define A3   GPIO_PC03
#define A4   GPIO_PC04  /**< TWI SDA. */
#define A5   GPIO_PC05  /**< TWI SCL. */

/* Board console UART — the ATmega328P has a single USART. */
#define BOARD_CONSOLE_UART      HAL_UART_0
#define BOARD_CONSOLE_UART_IRQ  HAL_IRQ_USART_RX

/* General-purpose timer — Timer1 (16-bit); Timer0 backs the timebase. */
#define BOARD_GP_TIMER  TIM1

/* PWM output — Timer1 channel 1 emits on OC1A (PB1). */
#define BOARD_PWM_TIMER    TIM1
#define BOARD_PWM_CHANNEL  1
#define BOARD_PWM_PIN      GPIO_PB01

/* I2C bus — the TWI peripheral on PC5 (SCL) / PC4 (SDA). */
#define BOARD_I2C_BUS  HAL_I2C_0
#define BOARD_I2C_SCL  GPIO_PC05
#define BOARD_I2C_SDA  GPIO_PC04

/* SPI bus — the SPI peripheral; CS uses PB2 (the hardware SS pin / D10). */
#define BOARD_SPI_BUS  HAL_SPI_0
#define BOARD_SPI_CS   GPIO_PB02

/* On-board oscillator (Hz) — the Arduino Uno uses a 16 MHz crystal. */
#define BOARD_XTAL_FREQ_HZ  16000000U

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_BOARD_ATMEGA328P_H */
