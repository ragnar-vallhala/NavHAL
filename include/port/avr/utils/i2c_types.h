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
 * @file i2c_types.h
 * @brief I²C bus-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single TWI (two-wire) peripheral, exposed as
 * ::HAL_I2C_0.
 */

#ifndef I2C_TYPES_H
#define I2C_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief I²C bus instance identifier (ATmega328P: the TWI peripheral). */
typedef enum {
  HAL_I2C_0 = 0, /**< TWI — the only I²C bus on the ATmega328P. */
} hal_i2c_bus_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* I2C_TYPES_H */
