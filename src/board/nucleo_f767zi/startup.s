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
 * @brief Startup + vector table for STM32F767ZI (Cortex-M7).
 *
 * @details
 * The Reset_Handler / .data-.bss init / Default_Handler-dispatch mechanism is
 * the same as the shared ARMv7E-M arch startup, but the **peripheral vector
 * table is MCU-specific** and the shared file carries the Nucleo-F401's map —
 * on which the F767's USART3 slot (IRQ 39) is "Reserved" (a literal 0). Enabling
 * an interrupt-driven peripheral the F401 lacks (e.g. USART3, the F767 console)
 * would then vector to address 0 and fault.
 *
 * This table follows the STM32F767xx layout (RM0410 Table 46). Every IRQ slot a
 * NavHAL driver could enable points to a real handler; the named handlers are
 * weak-aliased to Default_Handler, which tail-branches to the IPSR-based C
 * dispatcher (so any enabled line with a registered callback works without a
 * hand-written vector entry). Slots for peripherals NavHAL never enables also
 * point to Default_Handler rather than 0, so no usable IRQ can trap to address 0.
 *
 * Selected over the arch startup by the build when this file is present
 * (root CMakeLists.txt / src/vendor/stm32/CMakeLists.txt).
 */

.syntax unified
.cpu cortex-m7
.thumb

.global _estack
.global Reset_Handler
.global Default_Handler
.global SysTick_Handler
.global HardFault_Handler
.global PendSV_Handler
.global SVCall_Handler
/* DMA stream handlers (macro-driven in the DMA driver). */
.global DMA1_Stream0_IRQHandler
.global DMA1_Stream1_IRQHandler
.global DMA1_Stream2_IRQHandler
.global DMA1_Stream3_IRQHandler
.global DMA1_Stream4_IRQHandler
.global DMA1_Stream5_IRQHandler
.global DMA1_Stream6_IRQHandler
.global DMA1_Stream7_IRQHandler
.global DMA2_Stream0_IRQHandler
.global DMA2_Stream1_IRQHandler
.global DMA2_Stream2_IRQHandler
.global DMA2_Stream3_IRQHandler
.global DMA2_Stream4_IRQHandler
.global DMA2_Stream5_IRQHandler
.global DMA2_Stream6_IRQHandler
.global DMA2_Stream7_IRQHandler
/* Timers + UARTs NavHAL drivers touch. */
.global TIM1BRK_TIM9_IRQHandler
.global TIM2_IRQHandler
.global TIM3_IRQHandler
.global TIM4_IRQHandler
.global TIM5_IRQHandler
.global USART1_IRQHandler
.global USART2_IRQHandler
.global USART3_IRQHandler
.global USART6_IRQHandler
.global SDIO_IRQHandler

.section .isr_vector, "a", %progbits
    .word  _estack                  /* Top of Stack */
    .word  Reset_Handler            /* Reset */
    .word  Default_Handler          /* NMI */
    .word  HardFault_Handler        /* HardFault */
    .word  Default_Handler          /* MemManage */
    .word  Default_Handler          /* BusFault */
    .word  Default_Handler          /* UsageFault */
    .word  0                        /* Reserved */
    .word  0                        /* Reserved */
    .word  0                        /* Reserved */
    .word  0                        /* Reserved */
    .word  SVCall_Handler           /* SVCall */
    .word  Default_Handler          /* DebugMon */
    .word  0                        /* Reserved */
    .word  PendSV_Handler           /* PendSV */
    .word  SysTick_Handler          /* SysTick */

    /* External Interrupts — STM32F767xx (IRQ 0..) */
    .word  Default_Handler          /* 0  WWDG */
    .word  Default_Handler          /* 1  PVD */
    .word  Default_Handler          /* 2  TAMP_STAMP */
    .word  Default_Handler          /* 3  RTC_WKUP */
    .word  Default_Handler          /* 4  FLASH */
    .word  Default_Handler          /* 5  RCC */
    .word  Default_Handler          /* 6  EXTI0 */
    .word  Default_Handler          /* 7  EXTI1 */
    .word  Default_Handler          /* 8  EXTI2 */
    .word  Default_Handler          /* 9  EXTI3 */
    .word  Default_Handler          /* 10 EXTI4 */
    .word  DMA1_Stream0_IRQHandler  /* 11 DMA1 Stream0 */
    .word  DMA1_Stream1_IRQHandler  /* 12 DMA1 Stream1 */
    .word  DMA1_Stream2_IRQHandler  /* 13 DMA1 Stream2 */
    .word  DMA1_Stream3_IRQHandler  /* 14 DMA1 Stream3 */
    .word  DMA1_Stream4_IRQHandler  /* 15 DMA1 Stream4 */
    .word  DMA1_Stream5_IRQHandler  /* 16 DMA1 Stream5 */
    .word  DMA1_Stream6_IRQHandler  /* 17 DMA1 Stream6 */
    .word  Default_Handler          /* 18 ADC */
    .word  Default_Handler          /* 19 CAN1_TX */
    .word  Default_Handler          /* 20 CAN1_RX0 */
    .word  Default_Handler          /* 21 CAN1_RX1 */
    .word  Default_Handler          /* 22 CAN1_SCE */
    .word  Default_Handler          /* 23 EXTI9_5 */
    .word  TIM1BRK_TIM9_IRQHandler  /* 24 TIM1_BRK_TIM9 */
    .word  Default_Handler          /* 25 TIM1_UP_TIM10 */
    .word  Default_Handler          /* 26 TIM1_TRG_COM_TIM11 */
    .word  Default_Handler          /* 27 TIM1_CC */
    .word  TIM2_IRQHandler          /* 28 TIM2 */
    .word  TIM3_IRQHandler          /* 29 TIM3 */
    .word  TIM4_IRQHandler          /* 30 TIM4 */
    .word  Default_Handler          /* 31 I2C1_EV */
    .word  Default_Handler          /* 32 I2C1_ER */
    .word  Default_Handler          /* 33 I2C2_EV */
    .word  Default_Handler          /* 34 I2C2_ER */
    .word  Default_Handler          /* 35 SPI1 */
    .word  Default_Handler          /* 36 SPI2 */
    .word  USART1_IRQHandler        /* 37 USART1 */
    .word  USART2_IRQHandler        /* 38 USART2 */
    .word  USART3_IRQHandler        /* 39 USART3  <-- F401 had 0 here */
    .word  Default_Handler          /* 40 EXTI15_10 */
    .word  Default_Handler          /* 41 RTC_Alarm */
    .word  Default_Handler          /* 42 OTG_FS_WKUP */
    .word  Default_Handler          /* 43 TIM8_BRK_TIM12 */
    .word  Default_Handler          /* 44 TIM8_UP_TIM13 */
    .word  Default_Handler          /* 45 TIM8_TRG_COM_TIM14 */
    .word  Default_Handler          /* 46 TIM8_CC */
    .word  DMA1_Stream7_IRQHandler  /* 47 DMA1 Stream7 */
    .word  Default_Handler          /* 48 FMC */
    .word  SDIO_IRQHandler        /* 49 SDMMC1 */
    .word  TIM5_IRQHandler          /* 50 TIM5 */
    .word  Default_Handler          /* 51 SPI3 */
    .word  Default_Handler          /* 52 UART4 */
    .word  Default_Handler          /* 53 UART5 */
    .word  Default_Handler          /* 54 TIM6_DAC */
    .word  Default_Handler          /* 55 TIM7 */
    .word  DMA2_Stream0_IRQHandler  /* 56 DMA2 Stream0 */
    .word  DMA2_Stream1_IRQHandler  /* 57 DMA2 Stream1 */
    .word  DMA2_Stream2_IRQHandler  /* 58 DMA2 Stream2 */
    .word  DMA2_Stream3_IRQHandler  /* 59 DMA2 Stream3 */
    .word  DMA2_Stream4_IRQHandler  /* 60 DMA2 Stream4 */
    .word  Default_Handler          /* 61 ETH */
    .word  Default_Handler          /* 62 ETH_WKUP */
    .word  Default_Handler          /* 63 CAN2_TX */
    .word  Default_Handler          /* 64 CAN2_RX0 */
    .word  Default_Handler          /* 65 CAN2_RX1 */
    .word  Default_Handler          /* 66 CAN2_SCE */
    .word  Default_Handler          /* 67 OTG_FS */
    .word  DMA2_Stream5_IRQHandler  /* 68 DMA2 Stream5 */
    .word  DMA2_Stream6_IRQHandler  /* 69 DMA2 Stream6 */
    .word  DMA2_Stream7_IRQHandler  /* 70 DMA2 Stream7 */
    .word  USART6_IRQHandler        /* 71 USART6 */
    .word  Default_Handler          /* 72 I2C3_EV */
    .word  Default_Handler          /* 73 I2C3_ER */
    .word  Default_Handler          /* 74 OTG_HS_EP1_OUT */
    .word  Default_Handler          /* 75 OTG_HS_EP1_IN */
    .word  Default_Handler          /* 76 OTG_HS_WKUP */
    .word  Default_Handler          /* 77 OTG_HS */
    .word  Default_Handler          /* 78 DCMI */
    .word  Default_Handler          /* 79 CRYP */
    .word  Default_Handler          /* 80 HASH_RNG */
    .word  Default_Handler          /* 81 FPU */
    .word  Default_Handler          /* 82 UART7 */
    .word  Default_Handler          /* 83 UART8 */
    .word  Default_Handler          /* 84 SPI4 */
    .word  Default_Handler          /* 85 SPI5 */
    .word  Default_Handler          /* 86 SPI6 */
    .word  Default_Handler          /* 87 SAI1 */
    .word  Default_Handler          /* 88 LCD-TFT (LTDC) */
    .word  Default_Handler          /* 89 LCD-TFT error (LTDC_ER) */
    .word  Default_Handler          /* 90 DMA2D */
    .word  Default_Handler          /* 91 SAI2 */
    .word  Default_Handler          /* 92 QUADSPI */
    .word  Default_Handler          /* 93 LPTIM1 */
    .word  Default_Handler          /* 94 HDMI-CEC */
    .word  Default_Handler          /* 95 I2C4_EV */
    .word  Default_Handler          /* 96 I2C4_ER */
    .word  Default_Handler          /* 97 SPDIF_RX */

/*
 * @brief Reset Handler — copy .data, zero .bss, call main(). Identical to the
 * arch startup; duplicated so this file is a self-contained chip startup.
 */
.text
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_data:
    cmp r1, r2
    bcs init_bss
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data
init_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
zero_bss:
    cmp r0, r1
    bcs call_main
    movs r2, #0
    str r2, [r0], #4
    b zero_bss
call_main:
    cpsie i
    bl main
loop_forever:
    b loop_forever

.weak Default_Handler
.weak DMA1_Stream0_IRQHandler
.weak DMA1_Stream1_IRQHandler
.weak DMA1_Stream2_IRQHandler
.weak DMA1_Stream3_IRQHandler
.weak DMA1_Stream4_IRQHandler
.weak DMA1_Stream5_IRQHandler
.weak DMA1_Stream6_IRQHandler
.weak DMA1_Stream7_IRQHandler
.weak DMA2_Stream0_IRQHandler
.weak DMA2_Stream1_IRQHandler
.weak DMA2_Stream2_IRQHandler
.weak DMA2_Stream3_IRQHandler
.weak DMA2_Stream4_IRQHandler
.weak DMA2_Stream5_IRQHandler
.weak DMA2_Stream6_IRQHandler
.weak DMA2_Stream7_IRQHandler
.weak SDIO_IRQHandler
.weak USART1_IRQHandler
.weak USART2_IRQHandler
.weak USART3_IRQHandler
.weak USART6_IRQHandler
.weak TIM1BRK_TIM9_IRQHandler
.weak TIM2_IRQHandler
.weak TIM3_IRQHandler
.weak TIM4_IRQHandler
.weak TIM5_IRQHandler

.set DMA1_Stream0_IRQHandler, Default_Handler
.set DMA1_Stream1_IRQHandler, Default_Handler
.set DMA1_Stream2_IRQHandler, Default_Handler
.set DMA1_Stream3_IRQHandler, Default_Handler
.set DMA1_Stream4_IRQHandler, Default_Handler
.set DMA1_Stream5_IRQHandler, Default_Handler
.set DMA1_Stream6_IRQHandler, Default_Handler
.set DMA1_Stream7_IRQHandler, Default_Handler
.set DMA2_Stream0_IRQHandler, Default_Handler
.set DMA2_Stream1_IRQHandler, Default_Handler
.set DMA2_Stream2_IRQHandler, Default_Handler
.set DMA2_Stream3_IRQHandler, Default_Handler
.set DMA2_Stream4_IRQHandler, Default_Handler
.set DMA2_Stream5_IRQHandler, Default_Handler
.set DMA2_Stream6_IRQHandler, Default_Handler
.set DMA2_Stream7_IRQHandler, Default_Handler
.set SDIO_IRQHandler, Default_Handler
.set USART1_IRQHandler, Default_Handler
.set USART2_IRQHandler, Default_Handler
.set USART3_IRQHandler, Default_Handler
.set USART6_IRQHandler, Default_Handler
.set TIM1BRK_TIM9_IRQHandler, Default_Handler
.set TIM2_IRQHandler, Default_Handler
.set TIM3_IRQHandler, Default_Handler
.set TIM4_IRQHandler, Default_Handler
.set TIM5_IRQHandler, Default_Handler

.section .text.Default_Handler, "ax", %progbits
Default_Handler:
    /* Generic fallback: tail-branch to the C dispatcher, which reads IPSR and
       invokes the registered callback for the active IRQ, or traps on a
       genuinely unexpected exception. */
    b hal_irq_default_dispatch
