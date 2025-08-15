#ifndef CORTEX_M4_GPIO_REG_H
#define CORTEX_M4_GPIO_REG_H

/**
 * @file gpio_reg.h
 * @brief Register map and base addresses for GPIO ports on STM32F4 series.
 * @details
 * This header defines the memory-mapped structure for GPIO port registers,
 * as well as macros for accessing GPIO ports and pins on STM32F4 microcontrollers.
 *
 * The GPIO ports are located at fixed base addresses in the AHB1 peripheral space.
 * Each port consists of control registers for mode, type, speed, pull-up/pull-down,
 * input/output data, bit set/reset, lock, and alternate functions.
 */

#include "utils/types.h"
#include <stdint.h>

/**
 * @brief GPIO port register map structure.
 * @note Access via `GPIOx` pointers or macros such as `GPIO_GET_PORT(n)`.
 */
typedef struct {
  __IO uint32_t MODER;   /**< 0x00: GPIO port mode register. */
  __IO uint32_t OTYPER;  /**< 0x04: GPIO port output type register. */
  __IO uint32_t OSPEEDR; /**< 0x08: GPIO port output speed register. */
  __IO uint32_t PUPDR;   /**< 0x0C: GPIO port pull-up/pull-down register. */
  __IO uint32_t IDR;     /**< 0x10: GPIO port input data register. */
  __IO uint32_t ODR;     /**< 0x14: GPIO port output data register. */
  __IO uint32_t BSRR;    /**< 0x18: GPIO port bit set/reset register. */
  __IO uint32_t LCKR;    /**< 0x1C: GPIO port configuration lock register. */
  __IO uint32_t AFRL;    /**< 0x20: GPIO alternate function low register. */
  __IO uint32_t AFRH;    /**< 0x24: GPIO alternate function high register. */
} GPIOx_Typedef;

/** @brief Base address of GPIOA port registers. */
#define GPIOA_BASE_ADDR 0x40020000
/** @brief Base address of GPIOB port registers. */
#define GPIOB_BASE_ADDR 0x40020400
/** @brief Base address of GPIOC port registers. */
#define GPIOC_BASE_ADDR 0x40020800
/** @brief Base address of GPIOD port registers. */
#define GPIOD_BASE_ADDR 0x40020C00
/** @brief Base address of GPIOE port registers. */
#define GPIOE_BASE_ADDR 0x40021000
/** @brief Base address of GPIOH port registers. */
#define GPIOH_BASE_ADDR 0x40021C00

/**
 * @brief Get GPIO port number from pin index.
 * @param n Pin index (0..127).
 * @return Port number (0 for GPIOA, 1 for GPIOB, etc.).
 * @note Special case: port 5 maps to port H (index 7).
 */
#define GPIO_GET_PORT_NUMBER(n) (n / 16 == 5 ? 7 : n / 16)

/**
 * @brief Get GPIO port structure pointer from pin index.
 * @param n Pin index (0..127).
 * @return Pointer to the corresponding `GPIOx_Typedef`.
 * @code
 * // Example: set pin 5 (PA5) high
 * GPIO_GET_PORT(5)->BSRR = (1 << GPIO_GET_PIN(5));
 * @endcode
 */
#define GPIO_GET_PORT(n) \
  ((GPIOx_Typedef *)(GPIOA_BASE_ADDR + ((GPIO_GET_PORT_NUMBER(n)) * 0x400)))

/**
 * @brief Get pin number within its port.
 * @param n Pin index (0..127).
 * @return Pin number (0..15).
 */
#define GPIO_GET_PIN(n) (n % 16)

#endif // !CORTEX_M4_GPIO_REG_H
