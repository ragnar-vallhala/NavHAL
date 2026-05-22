/**
 * @file startup.s
 * @brief Startup code for ARM Cortex-M3 microcontroller.
 *
 * This file provides the vector table and the reset handler for system startup.
 * It includes routines for initializing the .data and .bss sections before
 * transferring control to the `main()` function.
 *
 * - Sets the initial stack pointer.
 * - Defines the Reset_Handler entry point.
 * - Copies initialized data from flash to RAM.
 * - Zeroes out the .bss section.
 * - Jumps to `main()`.
 *
 * Symbols referenced:
 *   - _estack: Top of the stack.
 *   - _sidata: Start of initialized data in FLASH.
 *   - _sdata, _edata: Start and end of initialized data in RAM.
 *   - _sbss, _ebss: Start and end of the BSS section.
 *
 * This file must be assembled and linked with the application.
 */

.syntax unified
.cpu cortex-m3
.thumb

.global SysTick_Handler
.global Reset_Handler
.global _estack

/**
 * @brief Vector Table
 * This section contains the vector table with the initial stack pointer
 * and the address of the Reset_Handler.
 */
.section .isr_vector, "a", %progbits
    .word  _estack                 /* 1. Top of Stack */
    .word  Reset_Handler           /* 2. Reset Handler */
    .word  Reset_Handler           /* 3. NMI Handler */
    .word  Reset_Handler           /* 4. Hard Fault Handler */
    .word  Reset_Handler           /* 5. MPU Fault Handler */
    .word  Reset_Handler           /* 6. Bus Fault Handler */
    .word  Reset_Handler           /* 7. Usage Fault Handler */
    .word  0                       /* 8. Reserved */
    .word  0                       /* 9. Reserved */
    .word  0                       /* 10. Reserved */
    .word  0                       /* 11. Reserved */
    .word  Reset_Handler           /* 12. SVCall Handler */
    .word  Reset_Handler           /* 13. Debug Monitor Handler */
    .word  0                       /* 14. Reserved */
    .word  Reset_Handler           /* 15. PendSV Handler */
    .word  SysTick_Handler         /* 16. SysTick Handler */
/*
 * @brief Reset Handler
 * This is the entry point after a reset. It copies initialized data from
 * flash to RAM, zeroes the .bss section, and then calls main().
 */
.text
.type Reset_Handler, %function
Reset_Handler:

    // Copy .data section from Flash to RAM
    ldr r0, =_sidata      // r0 = source (start of initialized data in Flash)
    ldr r1, =_sdata       // r1 = destination (start of .data in RAM)
    ldr r2, =_edata       // r2 = end of .data in RAM

copy_data:
    cmp r1, r2            // while (r1 < r2)
    bcc copy
    b init_bss            // done copying, proceed to zero .bss

copy:
    ldr r3, [r0], #4      // load word from r0 into r3, increment r0
    str r3, [r1], #4      // store r3 to r1, increment r1
    b copy_data

    // Zero initialize the .bss section (uninitialized data)
init_bss:
    ldr r0, =_sbss        // r0 = start of .bss
    ldr r1, =_ebss        // r1 = end of .bss

zero_bss:
    cmp r0, r1            // while (r0 < r1)
    bcc zero
    b call_main

zero:
    movs r2, #0
    str r2, [r0], #4      // store 0 to [r0], increment r0
    b zero_bss

    // Call main function
call_main:
    cpsie i  // enable interrupts
    bl main

    // If main returns, loop forever
loop_forever:
    b loop_forever
