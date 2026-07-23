/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file family/interrupt_reg.h
 * @brief PC interrupt lines (::hal_irq_t) — the 8259 PIC IRQ0..IRQ15.
 *
 * @details
 * The named lines are the fixed legacy-PC assignments. The 8259 PIC is remapped
 * to CPU vectors 32..47 (IRQ0 -> 32) by the interrupt driver so device IRQs do
 * not collide with CPU exception vectors 0..31.
 */

#ifndef NAVHAL_PC_INTERRUPT_REG_H
#define NAVHAL_PC_INTERRUPT_REG_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PIC interrupt lines (IRQ0..IRQ15). */
typedef enum {
  HAL_IRQ_TIMER = 0,    /**< IRQ0 — 8254 PIT. */
  HAL_IRQ_KEYBOARD = 1, /**< IRQ1 — PS/2 keyboard. */
  HAL_IRQ_CASCADE = 2,  /**< IRQ2 — slave PIC cascade. */
  HAL_IRQ_COM2 = 3,     /**< IRQ3 — COM2/COM4. */
  HAL_IRQ_COM1 = 4,     /**< IRQ4 — COM1/COM3. */
  HAL_IRQ_15 = 15,      /**< Highest PIC line. */
} hal_irq_t;

/** @brief Number of PIC interrupt lines. */
#define HAL_IRQ_COUNT 16

/** @brief CPU vector the PIC's IRQ0 is remapped to. */
#define HAL_IRQ_VECTOR_BASE 32

#ifdef __cplusplus
}
#endif

#endif /* NAVHAL_PC_INTERRUPT_REG_H */
