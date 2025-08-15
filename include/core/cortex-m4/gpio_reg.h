/**
 * @file core/cortex-m4/gpio_reg.h
 * @brief Cortex-M4 GPIO register definitions and access macros.
 *
 * @details
 * This header defines the GPIO register structure and macros to access
 * GPIO ports and pins on Cortex-M4 microcontrollers.
 *
 * Structures:
 * - `GPIOx_Typedef` : Represents the memory-mapped GPIO registers for a port.
 *
 * Macros:
 * - `GPIOA_BASE_ADDR` to `GPIOH_BASE_ADDR` : Base addresses of GPIO ports.
 * - `GPIO_GET_PORT_NUMBER(n)` : Compute the port number from a pin number.
 * - `GPIO_GET_PORT(n)` : Get a pointer to the GPIO port structure.
 * - `GPIO_GET_PIN(n)` : Get the pin number within a port.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_GPIO_REG_H
#define CORTEX_M4_GPIO_REG_H

#include "utils/types.h"
#include <stdint.h>

/**
 * @brief GPIO port register structure.
 *
 * @details
 * Represents all the standard registers for a GPIO port:
 * - MODER   : Mode register
 * - OTYPER  : Output type register
 * - OSPEEDR : Output speed register
 * - PUPDR   : Pull-up/Pull-down register
 * - IDR     : Input data register
 * - ODR     : Output data register
 * - BSRR    : Bit set/reset register
 * - LCKR    : Configuration lock register
 * - AFRL    : Alternate function low register
 * - AFRH    : Alternate function high register
 */
typedef struct {
    __IO uint32_t MODER;   /**< GPIO port mode register */
    __IO uint32_t OTYPER;  /**< GPIO output type register */
    __IO uint32_t OSPEEDR; /**< GPIO output speed register */
    __IO uint32_t PUPDR;   /**< GPIO pull-up/pull-down register */
    __IO uint32_t IDR;     /**< GPIO input data register */
    __IO uint32_t ODR;     /**< GPIO output data register */
    __IO uint32_t BSRR;    /**< GPIO bit set/reset register */
    __IO uint32_t LCKR;    /**< GPIO configuration lock register */
    __IO uint32_t AFRL;    /**< GPIO alternate function low register */
    __IO uint32_t AFRH;    /**< GPIO alternate function high register */
} GPIOx_Typedef;

/** Base addresses for GPIO ports */
#define GPIOA_BASE_ADDR 0x40020000 /**< GPIOA base address */
#define GPIOB_BASE_ADDR 0x40020400 /**< GPIOB base address */
#define GPIOC_BASE_ADDR 0x40020800 /**< GPIOC base address */
#define GPIOD_BASE_ADDR 0x40020C00 /**< GPIOD base address */
#define GPIOE_BASE_ADDR 0x40021000 /**< GPIOE base address */
#define GPIOH_BASE_ADDR 0x40021C00 /**< GPIOH base address */

/** Get port number from absolute pin number */
#define GPIO_GET_PORT_NUMBER(n) (n / 16 == 5 ? 7 : n / 16)

/** Get pointer to GPIO port structure from absolute pin number */
#define GPIO_GET_PORT(n) \
  ((GPIOx_Typedef *)(GPIOA_BASE_ADDR + ((GPIO_GET_PORT_NUMBER(n)) * 0x400)))

/** Get pin number within the port from absolute pin number */
#define GPIO_GET_PIN(n) (n % 16)

#endif // !CORTEX_M4_GPIO_REG_H
