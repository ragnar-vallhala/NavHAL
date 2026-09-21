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
 * @brief ACME1 GPIO register block.
 *
 * @details
 * Deliberately not shaped like the STM32 block: separate SET, CLR and TGL
 * registers rather than a half-word BSRR. The point of this port is that a
 * vendor describes its own silicon, so if this file had to imitate another
 * vendor to compile, the abstraction would be leaking.
 */

#ifndef ACME1_GPIO_REG_H
#define ACME1_GPIO_REG_H

#include <stdint.h>

/** @brief One GPIO port's registers. */
typedef struct {
  volatile uint32_t MODER;   /**< 2 bits per pin: mode select. */
  volatile uint32_t OTYPER;  /**< 1 bit per pin: push-pull / open-drain. */
  volatile uint32_t OSPEEDR; /**< 2 bits per pin: slew rate. */
  volatile uint32_t PUPDR;   /**< 2 bits per pin: pull-up / pull-down. */
  volatile uint32_t IDR;     /**< Input data, read-only. */
  volatile uint32_t ODR;     /**< Output data. */
  volatile uint32_t SET;     /**< Write-1-to-set; other bits unaffected. */
  volatile uint32_t CLR;     /**< Write-1-to-clear; other bits unaffected. */
  volatile uint32_t TGL;     /**< Write-1-to-toggle; atomic, unlike ODR ^=. */
  volatile uint32_t AFR[2];  /**< 4 bits per pin: alternate function. */
} GPIO_Reg_Typedef;

/** @brief Port base addresses, 1 KiB apart, ports A..H. */
#define ACME_GPIO_BASE 0x48000000UL
#define ACME_GPIO_PORT_STRIDE 0x400UL

/** @brief Clock gate for the GPIO ports, one enable bit per port. */
#define ACME_GPIO_CLKEN (*(volatile uint32_t *)0x40021000UL)

/* The two accessors the arch header calls. A pin id packs the port in the
 * high nibble and the pin number in the low nibble. */
#define GPIO_GET_PORT_NUMBER(n) ((n) >> 4)
#define GPIO_GET_PIN(n) ((n) & 0x0F)
#define GPIO_GET_PORT(n)                                                       \
  ((GPIO_Reg_Typedef *)(ACME_GPIO_BASE +                                       \
                        (ACME_GPIO_PORT_STRIDE * GPIO_GET_PORT_NUMBER(n))))

#endif /* ACME1_GPIO_REG_H */
