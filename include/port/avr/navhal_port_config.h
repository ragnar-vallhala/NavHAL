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
 * @file port/avr/navhal_port_config.h
 * @brief AVR / ATmega328P build-time feature flags.
 *
 * @details
 * Counterpart of the Cortex-M4 navhal_port_config.h. The ATmega328P has
 * none of the optional peripherals the HAL gates on — no DMA, no hardware
 * FPU, no DWT-style cycle counter, no SDIO, no hardware CRC — so this
 * header defines no `_*_ENABLED` flags. The DMA / SDIO API headers
 * therefore collapse to nothing, and `NAVHAL_HAS_*` resolve to 0.
 */

#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H

/* Intentionally empty: the ATmega328P exposes no gated optional peripheral. */

#endif /* NAVHAL_PORT_CONFIG_H */
