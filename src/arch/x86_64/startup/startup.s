/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * x86-64 bare-metal startup: multiboot2 header + long-mode bring-up.
 *
 * GRUB (multiboot2) loads this ELF64 and enters _start in 32-bit protected
 * mode with paging off. We identity-map the first 1 GiB with 2 MiB pages,
 * switch to long mode, turn the SSE unit on (the SysV C ABI uses xmm), then
 * call the sample's main(). No IDT/PIC yet — the first slice is polled I/O.
 */

.set MB2_MAGIC,  0xE85250D6
.set MB2_ARCH,   0                 /* 0 = i386 (32-bit protected mode) */

.section .multiboot, "a"
.align 8
mb2_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long mb2_end - mb2_start
    .long -(MB2_MAGIC + MB2_ARCH + (mb2_end - mb2_start))
    /* end tag */
    .short 0
    .short 0
    .long 8
mb2_end:

.section .bss
.align 4096
pml4:  .skip 4096
pdpt:  .skip 4096
pd:    .skip 4096
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .rodata
.align 8
gdt64:
    .quad 0                                            /* null descriptor */
gdt64_code = . - gdt64
    .quad (1<<43)|(1<<44)|(1<<47)|(1<<53)              /* 64-bit code segment */
gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global _start
_start:
    cli
    mov  $stack_top, %esp

    /* zero .bss (page tables + stack live here; done before they're used) */
    mov  $__bss_start, %edi
    mov  $__bss_end, %ecx
    sub  %edi, %ecx
    shr  $2, %ecx
    xor  %eax, %eax
    rep  stosl

    /* page tables: PML4[0] -> PDPT[0] -> PD, PD = 512 * 2 MiB identity pages */
    mov  $pdpt, %eax
    or   $0x3, %eax                    /* present | writable */
    mov  %eax, pml4
    mov  $pd, %eax
    or   $0x3, %eax
    mov  %eax, pdpt
    mov  $pd, %edi
    mov  $0x83, %eax                   /* present | writable | 2 MiB page */
    mov  $512, %ecx
1:
    mov  %eax, (%edi)
    add  $0x200000, %eax
    add  $8, %edi
    loop 1b

    mov  $pml4, %eax
    mov  %eax, %cr3

    mov  %cr4, %eax
    or   $(1<<5), %eax                 /* CR4.PAE */
    mov  %eax, %cr4

    mov  $0xC0000080, %ecx            /* IA32_EFER */
    rdmsr
    or   $(1<<8), %eax                 /* EFER.LME */
    wrmsr

    mov  %cr0, %eax
    or   $(1<<31), %eax                /* CR0.PG */
    mov  %eax, %cr0

    lgdt gdt64_pointer
    ljmp $gdt64_code, $long_mode

.code64
long_mode:
    xor  %ax, %ax
    mov  %ax, %ds
    mov  %ax, %es
    mov  %ax, %fs
    mov  %ax, %gs
    mov  %ax, %ss
    mov  $stack_top, %rsp

    /* enable SSE: clear CR0.EM, set CR0.MP, set CR4.OSFXSR|OSXMMEXCPT */
    mov  %cr0, %rax
    and  $~(1<<2), %rax
    or   $(1<<1), %rax
    mov  %rax, %cr0
    mov  %cr4, %rax
    or   $(3<<9), %rax
    mov  %rax, %cr4

    call main
.hang:
    cli
    hlt
    jmp  .hang

/* Mark the stack non-executable (silences the linker's default-exec-stack warning). */
.section .note.GNU-stack, "", @progbits
