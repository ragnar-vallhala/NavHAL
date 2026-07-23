/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file idt.c
 * @brief x86-64 Interrupt Descriptor Table setup (CPU-arch layer).
 *
 * @details
 * Builds the 256-gate IDT: every vector points at the halt-on-fault default
 * stub, then the 16 PIC lines (vectors 32..47) are pointed at the per-IRQ entry
 * stubs from isr.s. The platform interrupt driver (8259 PIC, hal_interrupt_*)
 * lives in the vendor layer and calls x86_idt_init() on first use.
 */

#include <stdint.h>

/** 64-bit interrupt-gate descriptor (16 bytes). */
typedef struct __attribute__((packed)) {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t flags;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
  uint16_t limit;
  uint64_t base;
} idt_ptr_t;

static idt_entry_t idt[256];

/* Entry stubs from isr.s. */
extern void x86_isr_default(void);
extern void *const x86_irq_stub_table[16];

/* Kernel code selector — index 1 in the long-mode GDT set up in startup.s. */
#define KERNEL_CS 0x08
/* Present | DPL0 | 64-bit interrupt gate. */
#define GATE_INT64 0x8E

static void set_gate(int vec, void (*handler)(void)) {
  uint64_t addr = (uint64_t)handler;
  idt[vec].offset_low = addr & 0xFFFF;
  idt[vec].selector = KERNEL_CS;
  idt[vec].ist = 0;
  idt[vec].flags = GATE_INT64;
  idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
  idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vec].zero = 0;
}

void x86_idt_init(void) {
  for (int v = 0; v < 256; v++)
    set_gate(v, x86_isr_default);
  for (int i = 0; i < 16; i++)
    set_gate(32 + i, (void (*)(void))x86_irq_stub_table[i]);

  idt_ptr_t idtr = {.limit = sizeof(idt) - 1, .base = (uint64_t)&idt};
  __asm__ volatile("lidt %0" : : "m"(idtr));
}
