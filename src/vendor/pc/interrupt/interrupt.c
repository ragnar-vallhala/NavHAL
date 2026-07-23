/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file interrupt.c
 * @brief HAL interrupt driver for the PC — 8259 PIC + IDT wiring.
 *
 * @details
 * On first use it loads the IDT (arch layer) and remaps the master/slave 8259
 * PICs to CPU vectors 32..47 with all lines masked, then enables interrupts.
 * hal_interrupt_enable/disable un/mask individual lines; x86_irq_dispatch (from
 * the ISR stubs) invokes the registered callback and sends the End-Of-Interrupt.
 */

#include "common/hal_interrupt.h"
#include "pc_io.h"
#include <stdbool.h>
#include <stddef.h>

/* 8259 PIC I/O ports. */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

extern void x86_idt_init(void); /* arch layer (idt.c) */

static hal_interrupt_callback_t g_cb[HAL_IRQ_COUNT];
static bool g_inited;

static void pic_remap(void) {
  /* ICW1: begin init, expect ICW4. */
  pc_outb(PIC1_CMD, 0x11);
  pc_outb(PIC2_CMD, 0x11);
  /* ICW2: vector offsets (master -> 32, slave -> 40). */
  pc_outb(PIC1_DATA, 32);
  pc_outb(PIC2_DATA, 40);
  /* ICW3: cascade wiring (slave on master IRQ2). */
  pc_outb(PIC1_DATA, 0x04);
  pc_outb(PIC2_DATA, 0x02);
  /* ICW4: 8086 mode. */
  pc_outb(PIC1_DATA, 0x01);
  pc_outb(PIC2_DATA, 0x01);
  /* Mask every line; hal_interrupt_enable unmasks per line. */
  pc_outb(PIC1_DATA, 0xFF);
  pc_outb(PIC2_DATA, 0xFF);
}

static void ensure_init(void) {
  if (g_inited) return;
  g_inited = true;
  x86_idt_init();
  pic_remap();
  __asm__ volatile("sti");
}

/* Called from the ISR stubs (isr.s) with the IRQ line number. */
void x86_irq_dispatch(uint64_t irq) {
  if (irq < HAL_IRQ_COUNT && g_cb[irq]) g_cb[irq]();
  /* End-Of-Interrupt: slave first (if applicable), then master. */
  if (irq >= 8) pc_outb(PIC2_CMD, PIC_EOI);
  pc_outb(PIC1_CMD, PIC_EOI);
}

static void set_mask(uint8_t irq, bool masked) {
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = irq & 7;
  uint8_t val = pc_inb(port);
  if (masked)
    val |= (uint8_t)(1u << bit);
  else
    val &= (uint8_t)~(1u << bit);
  pc_outb(port, val);
}

hal_status_t hal_interrupt_enable(hal_irq_t irq) {
  if ((unsigned)irq >= HAL_IRQ_COUNT) return HAL_ERR_INVALID_ARG;
  ensure_init();
  if (irq >= 8) set_mask(HAL_IRQ_CASCADE, false); /* slave needs the cascade line */
  set_mask((uint8_t)irq, false);
  return HAL_OK;
}

hal_status_t hal_interrupt_disable(hal_irq_t irq) {
  if ((unsigned)irq >= HAL_IRQ_COUNT) return HAL_ERR_INVALID_ARG;
  ensure_init();
  set_mask((uint8_t)irq, true);
  return HAL_OK;
}

hal_status_t hal_interrupt_attach_callback(hal_irq_t irq,
                                           hal_interrupt_callback_t cb) {
  if ((unsigned)irq >= HAL_IRQ_COUNT) return HAL_ERR_INVALID_ARG;
  g_cb[irq] = cb;
  return HAL_OK;
}

hal_status_t hal_interrupt_detach_callback(hal_irq_t irq) {
  if ((unsigned)irq >= HAL_IRQ_COUNT) return HAL_ERR_INVALID_ARG;
  g_cb[irq] = NULL;
  return HAL_OK;
}
