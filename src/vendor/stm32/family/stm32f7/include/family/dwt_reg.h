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
 * @file dwt_reg.h
 * @brief Cortex-M4 DWT (Data Watchpoint and Trace) register definitions.
 *
 * @details
 * This header defines the memory-mapped structure for DWT registers,
 * including cycle count, CPI count, and other performance counters.
 * It also defines the CoreDebug register for enabling the trace unit.
 *
 * @note DWT base address is 0xE0001000UL.
 * @note CoreDebug base address is 0xE000EDF0UL.
 */

#ifndef CORTEX_M4_DWT_REG_H
#define CORTEX_M4_DWT_REG_H

/* DWT and CoreDebug are Cortex core blocks at fixed architectural addresses,
 * so they moved to arch/armv7e-m/core_reg.h. This header remains as the name
 * the arch DWT driver includes. */
#include "arch/armv7e-m/core_reg.h"

#endif /* CORTEX_M4_DWT_REG_H */
