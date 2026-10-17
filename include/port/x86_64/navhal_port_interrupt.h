/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/navhal_port_interrupt.h
 * @brief x86-64 PC interrupt-controller (8259 PIC) HAL interface.
 *
 * @details
 * Implements the core `hal_interrupt_*` contract against the legacy 8259 PIC.
 * IRQ lines are @c hal_irq_t (see @c family/interrupt_reg.h). The IDT and ISR entry
 * stubs are the CPU-arch layer (src/arch/x86_64/interrupt); this API is the
 * platform interrupt controller — enabling/disabling lines and dispatching to
 * registered callbacks.
 *
 * The PIC has fixed hardware priority (IRQ0 highest), so there is no
 * settable-priority entry point as on the Cortex-M NVIC.
 */

#ifndef NAVHAL_PORT_X86_64_INTERRUPT_H
#define NAVHAL_PORT_X86_64_INTERRUPT_H

#include "common/hal_status.h"
#include "family/interrupt_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback invoked from the ISR for a registered IRQ line. */
typedef void (*hal_interrupt_callback_t)(void);





#ifdef __cplusplus
}
#endif

#endif /* NAVHAL_PORT_X86_64_INTERRUPT_H */
