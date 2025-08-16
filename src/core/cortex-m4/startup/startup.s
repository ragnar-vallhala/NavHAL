/**
 * @file startup.s
 * @brief Startup code for ARM Cortex-M3 microcontroller
 * @details
 * This file provides the vector table and the reset handler for system startup.
 * It includes routines for initializing the .data and .bss sections before
 * transferring control to the `main()` function.
 *
 * Key functionality:
 * - Sets the initial stack pointer
 * - Defines the Reset_Handler entry point
 * - Copies initialized data from flash to RAM
 * - Zeroes out the .bss section
 * - Jumps to `main()`
 * - Provides default interrupt handlers
 *
 * @note This file must be assembled and linked with the application
 * @copyright © NAVROBOTEC PVT. LTD.
 */

.syntax unified
.cpu cortex-m3
.thumb

/* Global symbol declarations */
.global _estack
.global Reset_Handler
.global SysTick_Handler
.global TIM5_IRQHandler
.global TIM1BRK_TIM9_IRQHandler
.global TIM2_IRQHandler 
.global TIM3_IRQHandler 
.global TIM4_IRQHandler 

/**
 * @brief Vector Table Section
 * @details
 * The vector table contains the initial stack pointer value and
 * addresses of all exception and interrupt handlers.
 * The first word is the initial stack pointer value.
 * Subsequent words are handler addresses for:
 * - Reset handler
 * - NMI handler
 * - Hard fault handler
 * - MPU fault handler
 * - Bus fault handler
 * - Usage fault handler
 * - SVCall handler
 * - Debug monitor handler
 * - PendSV handler
 * - SysTick handler
 * - External interrupts (IRQn)
 */
.section .isr_vector, "a", %progbits
    .word  _estack                  /* 1. Top of Stack */
    .word  Reset_Handler            /* 2. Reset Handler */
    .word  Reset_Handler            /* 3. NMI Handler */
    .word  Reset_Handler            /* 4. Hard Fault Handler */
    .word  Reset_Handler            /* 5. MPU Fault Handler */
    .word  Reset_Handler            /* 6. Bus Fault Handler */
    .word  Reset_Handler            /* 7. Usage Fault Handler */
    .word  0                        /* 8. Reserved */
    .word  0                        /* 9. Reserved */
    .word  0                        /* 10. Reserved */
    .word  0                        /* 11. Reserved */
    .word  Reset_Handler            /* 12. SVCall Handler */
    .word  Reset_Handler            /* 13. Debug Monitor Handler */
    .word  0                        /* 14. Reserved */
    .word  Reset_Handler            /* 15. PendSV Handler */
    .word  SysTick_Handler          /* 16. SysTick Handler */

    /* External Interrupts (IRQn) */
    .word  Reset_Handler            /* 0. Window Watchdog */
    .word  Reset_Handler            /* 1. EXTI Line 16/PVD through EXTI Line detect */
    /* ... (remaining vector table entries) */
    .word  Reset_Handler            /* 98. SPI4 */

/**
 * @brief Reset Handler
 * @details
 * This is the entry point after a reset. It performs the following:
 * 1. Copies initialized data from flash to RAM (.data section)
 * 2. Zeroes the .bss section
 * 3. Enables interrupts
 * 4. Calls main()
 * 5. Enters infinite loop if main() returns
 *
 * @note Uses registers r0-r3 for data initialization
 */
.text
.type Reset_Handler, %function
Reset_Handler:

    /* Copy .data section from Flash to RAM */
    ldr r0, =_sidata      /* r0 = source (start of initialized data in Flash) */
    ldr r1, =_sdata       /* r1 = destination (start of .data in RAM) */
    ldr r2, =_edata       /* r2 = end of .data in RAM */

copy_data:
    cmp r1, r2            /* while (r1 < r2) */
    bcc copy
    b init_bss            /* done copying, proceed to zero .bss */

copy:
    ldr r3, [r0], #4      /* load word from r0 into r3, increment r0 */
    str r3, [r1], #4      /* store r3 to r1, increment r1 */
    b copy_data

    /* Zero initialize the .bss section (uninitialized data) */
init_bss:
    ldr r0, =_sbss        /* r0 = start of .bss */
    ldr r1, =_ebss        /* r1 = end of .bss */

zero_bss:
    cmp r0, r1            /* while (r0 < r1) */
    bcc zero
    b call_main

zero:
    movs r2, #0
    str r2, [r0], #4      /* store 0 to [r0], increment r0 */
    b zero_bss

    /* Call main function */
call_main:
    cpsie i               /* enable interrupts */
    bl main

    /* If main returns, loop forever */
loop_forever:
    b loop_forever