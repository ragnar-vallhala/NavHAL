/**
 * @file board.h
 * @brief Board-layer aliases for the ST Nucleo-F401RE.
 *
 * @details
 * Resolves the portable, board-agnostic identifiers used by application code
 * (`LED_BUILTIN`, `USER_BUTTON`, `D0`..`D15`, `A0`..`A5`) into the per-MCU
 * core GPIO enum (`GPIO_PA05`, ...) defined for STM32F401RE. The same names
 * exist on every board that ships a `board.h`, so a sample written against
 * `LED_BUILTIN` can move between boards with only a board switch.
 *
 * Also exposes the on-board oscillator frequencies — HSE comes from the
 * ST-LINK MCO on the Nucleo, not a discrete crystal.
 *
 * Reference: UM1724 (Nucleo-64 user manual), §6 (hardware layout).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_BOARD_NUCLEO_F401RE_H
#define NAVHAL_BOARD_NUCLEO_F401RE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/gpio_types.h"
#include "utils/i2c_types.h"
#include "utils/spi_types.h"
#include "utils/timer_types.h"
#include "utils/uart_types.h"

/* On-board indicators / inputs */
#define LED_BUILTIN  GPIO_PA05  /**< LD2 (green user LED), shared with D13. */
#define USER_BUTTON  GPIO_PC13  /**< B1 user button, active-low. */

/* Board console UART — USART2 is wired to the ST-LINK virtual COM port. */
#define BOARD_CONSOLE_UART      HAL_UART_2
#define BOARD_CONSOLE_UART_IRQ  USART2_IRQn

/* General-purpose timer. */
#define BOARD_GP_TIMER  TIM5

/* PWM output — TIM2 channel 3 on PB10 (alternate function AF1). */
#define BOARD_PWM_TIMER    TIM2
#define BOARD_PWM_CHANNEL  3
#define BOARD_PWM_PIN      GPIO_PB10

/* I2C bus — I2C1 on PB8 (SCL) / PB9 (SDA). */
#define BOARD_I2C_BUS  HAL_I2C_1
#define BOARD_I2C_SCL  GPIO_PB08
#define BOARD_I2C_SDA  GPIO_PB09

/* SPI bus — SPI1; CS on PA4. */
#define BOARD_SPI_BUS  HAL_SPI_1
#define BOARD_SPI_CS   GPIO_PA04

/* Arduino-compatible digital headers (CN5/CN9 on the Nucleo-64) */
#define D0   GPIO_PA03  /**< USART2 RX on the Arduino header. */
#define D1   GPIO_PA02  /**< USART2 TX on the Arduino header. */
#define D2   GPIO_PA10
#define D3   GPIO_PB03
#define D4   GPIO_PB05
#define D5   GPIO_PB04
#define D6   GPIO_PB10
#define D7   GPIO_PA08
#define D8   GPIO_PA09
#define D9   GPIO_PC07
#define D10  GPIO_PB06  /**< SPI1 CS (default). */
#define D11  GPIO_PA07  /**< SPI1 MOSI. */
#define D12  GPIO_PA06  /**< SPI1 MISO. */
#define D13  GPIO_PA05  /**< SPI1 SCK; also LD2 / LED_BUILTIN. */
#define D14  GPIO_PB09  /**< I2C1 SDA. */
#define D15  GPIO_PB08  /**< I2C1 SCL. */

/* Arduino-compatible analog headers (CN8) */
#define A0   GPIO_PA00
#define A1   GPIO_PA01
#define A2   GPIO_PA04
#define A3   GPIO_PB00
#define A4   GPIO_PC01
#define A5   GPIO_PC00

/* On-board oscillator frequencies (Hz) */
#define BOARD_HSI_FREQ_HZ  16000000U  /**< Internal RC, fixed in silicon. */
#define BOARD_HSE_FREQ_HZ   8000000U  /**< MCO from the ST-LINK MCU. */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_BOARD_NUCLEO_F401RE_H */
