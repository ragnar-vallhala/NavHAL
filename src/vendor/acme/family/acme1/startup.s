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
 * @file startup.s
 * @brief ACME1 vector table.
 *
 * @details
 * The other half of what a port supplies, alongside its ops tables. Entries
 * 1..16 are the ARMv7E-M core exceptions and are the same on any Cortex-M4;
 * everything after is this MCU's own, which is exactly why the table lives
 * with the family rather than in the arch tree.
 *
 * ACME1 has one peripheral worth a vector -- a GPIO edge interrupt -- so the
 * table is short. A real port lists every line its reference manual defines.
 *
 * Reset_Handler itself is arch code (src/arch/armv7e-m/startup/boot.c): the
 * copy/zero table walk is identical on every ARMv7E-M part.
 */

.syntax unified
.cpu cortex-m4
.thumb

.global _estack
.global Reset_Handler
.global NMI_Handler
.global HardFault_Handler
.global MemManage_Handler
.global BusFault_Handler
.global UsageFault_Handler
.global SVCall_Handler
.global DebugMon_Handler
.global PendSV_Handler
.global SysTick_Handler
.global GPIO_IRQHandler

.section .isr_vector, "a", %progbits
    .word  _estack                  /* 1. Top of stack */
    .word  Reset_Handler            /* 2. Reset */
    .word  NMI_Handler              /* 3. NMI */
    .word  HardFault_Handler        /* 4. Hard fault */
    .word  MemManage_Handler        /* 5. MPU fault */
    .word  BusFault_Handler         /* 6. Bus fault */
    .word  UsageFault_Handler       /* 7. Usage fault */
    .word  0                        /* 8. Reserved */
    .word  0                        /* 9. Reserved */
    .word  0                        /* 10. Reserved */
    .word  0                        /* 11. Reserved */
    .word  SVCall_Handler           /* 12. SVCall */
    .word  DebugMon_Handler         /* 13. Debug monitor */
    .word  0                        /* 14. Reserved */
    .word  PendSV_Handler           /* 15. PendSV */
    .word  SysTick_Handler          /* 16. SysTick */

    /* External interrupts (IRQn) */
    .word  GPIO_IRQHandler          /* 0. GPIO edge detect */

/* Anything the firmware does not define falls back to a spin, so an
 * unexpected vector stops the part rather than running on. */
.weak NMI_Handler
.weak HardFault_Handler
.weak MemManage_Handler
.weak BusFault_Handler
.weak UsageFault_Handler
.weak SVCall_Handler
.weak DebugMon_Handler
.weak PendSV_Handler
.weak SysTick_Handler
.weak GPIO_IRQHandler

.set NMI_Handler, Default_Handler
.set HardFault_Handler, Default_Handler
.set MemManage_Handler, Default_Handler
.set BusFault_Handler, Default_Handler
.set UsageFault_Handler, Default_Handler
.set SVCall_Handler, Default_Handler
.set DebugMon_Handler, Default_Handler
.set PendSV_Handler, Default_Handler
.set SysTick_Handler, Default_Handler
.set GPIO_IRQHandler, Default_Handler

.section .text
.weak Default_Handler
.type Default_Handler, %function
Default_Handler:
    b .
.size Default_Handler, . - Default_Handler
