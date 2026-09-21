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
 * The field names here are not a free choice. @c navhal_port_gpio.h -- an
 * *arch* header, shared by every Cortex-M4 vendor -- implements the inlined
 * hot path (::hal_gpio_write, ::hal_gpio_read, ::hal_gpio_toggle) directly
 * against @c BSRR / @c IDR / @c ODR and the two accessor macros below. A
 * vendor whose GPIO block is shaped differently cannot express itself here;
 * it would have to emulate this layout or fork the arch header.
 *
 * That is a real constraint on the M9 exit criterion: the ops table is not
 * the whole vendor surface, because the hot path was deliberately left out of
 * the table and inlined at the arch layer instead.
 */

#ifndef ACME1_GPIO_REG_H
#define ACME1_GPIO_REG_H

#include <stdint.h>

/** @brief One GPIO port's registers. Field names fixed by the arch header. */
typedef struct {
  volatile uint32_t MODER;   /**< 2 bits per pin: mode select. */
  volatile uint32_t OTYPER;  /**< 1 bit per pin: push-pull / open-drain. */
  volatile uint32_t OSPEEDR; /**< 2 bits per pin: slew rate. */
  volatile uint32_t PUPDR;   /**< 2 bits per pin: pull-up / pull-down. */
  volatile uint32_t IDR;     /**< Input data, read-only. */
  volatile uint32_t ODR;     /**< Output data. */
  volatile uint32_t BSRR;    /**< Set in the low half, reset in the high half. */
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
