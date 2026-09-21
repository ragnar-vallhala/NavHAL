/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * x86-64 interrupt entry stubs.
 *
 * One stub per PIC line (IRQ0..IRQ15 -> vectors 32..47). Each pushes its IRQ
 * number and falls into a common path that saves the volatile registers, aligns
 * the stack, and calls the C dispatcher x86_irq_dispatch(irq). A default stub
 * catches every other vector (CPU exceptions, unused) with a clean halt so a
 * stray fault freezes instead of triple-faulting into a reboot loop.
 *
 * x86_irq_stub_table[] lets the C IDT setup install the 16 gates in a loop.
 */

.section .text
.code64

.macro IRQ_STUB n
.global x86_irq\n
x86_irq\n:
    pushq $\n
    jmp   irq_common
.endm

IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 2
IRQ_STUB 3
IRQ_STUB 4
IRQ_STUB 5
IRQ_STUB 6
IRQ_STUB 7
IRQ_STUB 8
IRQ_STUB 9
IRQ_STUB 10
IRQ_STUB 11
IRQ_STUB 12
IRQ_STUB 13
IRQ_STUB 14
IRQ_STUB 15

irq_common:
    push %rbx                 /* callee-saved: reused as an rsp save slot */
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov  80(%rsp), %rdi       /* pushed IRQ number (rbx + 9 volatiles below it) */
    mov  %rsp, %rbx           /* preserve rsp across the (aligning) call */
    and  $-16, %rsp           /* SysV: 16-byte stack alignment before call */
    cld
    call x86_irq_dispatch
    mov  %rbx, %rsp
    pop  %r11
    pop  %r10
    pop  %r9
    pop  %r8
    pop  %rdi
    pop  %rsi
    pop  %rdx
    pop  %rcx
    pop  %rax
    pop  %rbx
    add  $8, %rsp             /* discard the pushed IRQ number */
    iretq

.global x86_isr_default
x86_isr_default:
    cli
    hlt
    jmp  x86_isr_default

.section .rodata
.align 8
.global x86_irq_stub_table
x86_irq_stub_table:
    .quad x86_irq0,  x86_irq1,  x86_irq2,  x86_irq3
    .quad x86_irq4,  x86_irq5,  x86_irq6,  x86_irq7
    .quad x86_irq8,  x86_irq9,  x86_irq10, x86_irq11
    .quad x86_irq12, x86_irq13, x86_irq14, x86_irq15

.section .note.GNU-stack, "", @progbits
