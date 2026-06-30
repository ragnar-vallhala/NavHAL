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
 * @file family/gpio_reg.h
 * @brief STM32F7 GPIO register definitions and access macros.
 *
 * @details
 * The GPIO IP is register-compatible with the STM32F4 (RM0410 §6), so the
 * port register structure is identical. The difference from the F4 header is
 * the **port indexing**: the F767ZI exposes contiguous ports A–G (plus H for
 * the oscillator pins), so the port number is a straight `pin >> 4` and the
 * base address is `0x40020000 + port * 0x400` — no PE→PH remap quirk like the
 * F401's header carries.
 */

#ifndef CORTEX_M7_GPIO_REG_H
#define CORTEX_M7_GPIO_REG_H

#include "common/hal_types.h"
#include "utils/types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief GPIO port register structure (same layout as STM32F4).
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

/** Base addresses for GPIO ports (AHB1, contiguous 0x400 stride). */
#define GPIOA_BASE_ADDR 0x40020000 /**< GPIOA base address */
#define GPIOB_BASE_ADDR 0x40020400 /**< GPIOB base address */
#define GPIOC_BASE_ADDR 0x40020800 /**< GPIOC base address */
#define GPIOD_BASE_ADDR 0x40020C00 /**< GPIOD base address */
#define GPIOE_BASE_ADDR 0x40021000 /**< GPIOE base address */
#define GPIOF_BASE_ADDR 0x40021400 /**< GPIOF base address */
#define GPIOG_BASE_ADDR 0x40021800 /**< GPIOG base address */
#define GPIOH_BASE_ADDR 0x40021C00 /**< GPIOH base address */

/** Get port number (0=A .. 7=H) from absolute pin number. */
#define GPIO_GET_PORT_NUMBER(n) ((n) >> 4)

/** Get pointer to GPIO port structure from absolute pin number. */
#define GPIO_GET_PORT(n)                                                       \
  ((GPIOx_Typedef *)(GPIOA_BASE_ADDR + (GPIO_GET_PORT_NUMBER(n) << 10)))

/** Get pin number within the port from absolute pin number. */
#define GPIO_GET_PIN(n) ((n) & 0x0F)


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !CORTEX_M7_GPIO_REG_H
