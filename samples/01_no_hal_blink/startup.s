.syntax unified
.cpu cortex-m3
.thumb

.global Reset_Handler
.global _estack

// Vector table
.section .isr_vector, "a", %progbits
.word _estack             // Initial stack pointer
.word Reset_Handler       // Reset handler

// Reset handler
.text
.type Reset_Handler, %function
Reset_Handler:
    // Copy .data section from flash to RAM
    ldr r0, =_sidata      // source: initialized data in FLASH
    ldr r1, =_sdata       // destination: start of .data in RAM
    ldr r2, =_edata       // end of .data in RAM

copy_data:
    cmp r1, r2
    bcc copy              // if r1 < r2
    b init_bss

copy:
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data

// Zero initialize .bss section
init_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss

zero_bss:
    cmp r0, r1
    bcc zero
    b call_main

zero:
    movs r2, #0
    str r2, [r0], #4
    b zero_bss

// Call main()
call_main:
    bl main

loop_forever:
    b loop_forever
