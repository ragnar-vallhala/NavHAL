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
.cpu cortex-m4
.thumb



// Interrupt stubs

.global _estack
.global Reset_Handler
.global SysTick_Handler
.global PendSV_Handler
.global TIM5_IRQHandler
.global TIM1BRK_TIM9_IRQHandler
.global TIM2_IRQHandler 
.global TIM3_IRQHandler 
.global TIM4_IRQHandler 


/**
 * @brief Vector Table
 * This section contains the vector table with the initial stack pointer
 * and the address of the Reset_Handler.
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
    .word  PendSV_Handler            /* 15. PendSV Handler */
    .word  SysTick_Handler          /* 16. SysTick Handler */

    /* External Interrupts (IRQn) */
    .word  Reset_Handler            /* 0. Window Watchdog */
    .word  Reset_Handler            /* 1. EXTI Line 16/PVD through EXTI Line detect */
    .word  Reset_Handler            /* 2. EXTI Line 21/Tamper and TimeStamp */
    .word  Reset_Handler            /* 3. EXTI Line 22/RTC Wakeup */
    .word  Reset_Handler            /* 4. FLASH */
    .word  Reset_Handler            /* 5. RCC */
    .word  Reset_Handler            /* 6. EXTI Line0 */
    .word  Reset_Handler            /* 7. EXTI Line1 */
    .word  Reset_Handler            /* 8. EXTI Line2 */
    .word  Reset_Handler            /* 9. EXTI Line3 */
    .word  Reset_Handler            /* 10. EXTI Line4 */
    .word  Reset_Handler            /* 11. DMA1 Stream 0 */
    .word  Reset_Handler            /* 12. DMA1 Stream 1 */
    .word  Reset_Handler            /* 13. DMA1 Stream 2 */
    .word  Reset_Handler            /* 14. DMA1 Stream 3 */
    .word  Reset_Handler            /* 15. DMA1 Stream 4 */
    .word  Reset_Handler            /* 16. DMA1 Stream 5 */
    .word  Reset_Handler            /* 17. DMA1 Stream 6 */
    .word  Reset_Handler            /* 18. ADC1 */
    .word  0                        /* 19. Reserved */
    .word  0                        /* 20. Reserved */
    .word  0                        /* 21. Reserved */
    .word  0                        /* 22. Reserved */
    .word  Reset_Handler            /* 23. EXTI Line[9:5] */
    .word  TIM1BRK_TIM9_IRQHandler            /* 24. TIM1 Break & TIM9 */
    .word  Reset_Handler            /* 25. TIM1 Update & TIM10 */
    .word  Reset_Handler            /* 26. TIM1 Trig/Com & TIM11 */
    .word  Reset_Handler            /* 27. TIM1 Capture Compare */
    .word  TIM2_IRQHandler            /* 28. TIM2 */
    .word  TIM3_IRQHandler            /* 29. TIM3 */
    .word  TIM4_IRQHandler            /* 30. TIM4 */
    .word  Reset_Handler            /* 31. I2C1 Event */
    .word  Reset_Handler            /* 32. I2C1 Error */
    .word  Reset_Handler            /* 33. I2C2 Event */
    .word  Reset_Handler            /* 34. I2C2 Error */
    .word  Reset_Handler            /* 35. SPI1 */
    .word  Reset_Handler            /* 36. SPI2 */
    .word  Reset_Handler            /* 37. USART1 */
    .word  Reset_Handler            /* 38. USART2 */
    .word  0                        /* 39. Reserved */
    .word  Reset_Handler            /* 40. EXTI Line[15:10] */
    .word  Reset_Handler            /* 41. EXTI line 17/RTC Alarm (A and B) */
    .word  Reset_Handler            /* 42.EXTI Line 18/ USB OTG FS Wakeup */
    .word  0                        /* 43. Reserved */
    .word  0                        /* 44. Reserved */
    .word  0                        /* 45. Reserved */
    .word  0                        /* 46. Reserved */
    .word  Reset_Handler            /* 47. DMA1 Stream 7 */
    .word  0                        /* 48. Reserved */
    .word  Reset_Handler            /* 49. SDIO */
    .word  TIM5_IRQHandler            /* 50. TIM5 */
    .word  Reset_Handler            /* 51. SPI3 */
    /*Done till above*/
    .word  0                       /* 69. Reserved */
    .word  0                       /* 70. Reserved */
    .word  0                       /* 71. Reserved */
    .word  0                       /* 72. Reserved */
    .word  Reset_Handler /* 73. DMA2 Stream 0 */
    .word  Reset_Handler /* 74. DMA2 Stream 1 */
    .word  Reset_Handler /* 75. DMA2 Stream 2 */
    .word  Reset_Handler /* 76. DMA2 Stream 3 */
    .word  Reset_Handler /* 77. DMA2 Stream 4 */
    .word  0                       /* 78. Reserved */
    .word  0                       /* 79. Reserved */
    .word  0                       /* 80. Reserved */
    .word  0                       /* 81. Reserved */
    .word  0                       /* 82. Reserved */
    .word  0                       /* 83. Reserved */
    .word  Reset_Handler /* 84. USB OTG FS */
    .word  Reset_Handler /* 85. DMA2 Stream 5 */
    .word  Reset_Handler /* 86. DMA2 Stream 6 */
    .word  Reset_Handler /* 87. DMA2 Stream 7 */
    .word  Reset_Handler /* 88. USART6 */
    .word  Reset_Handler /* 89. I2C3 Event */
    .word  Reset_Handler /* 90. I2C3 Error */
    .word  0                       /* 91. Reserved */
    .word  0                       /* 92. Reserved */
    .word  0                       /* 93. Reserved */
    .word  0                       /* 94. Reserved */
    .word  Reset_Handler /* 95. FPU */
    .word  0                       /* 96. Reserved */
    .word  0                       /* 97. Reserved */
    .word  Reset_Handler /* 98. SPI4 */


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
    bcs copy
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
    bcs zero
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
