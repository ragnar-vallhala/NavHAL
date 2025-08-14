/**
 * @file startup.c
 * @brief Cortex-M4 startup code for STM32F4 series.
 *
 * This file provides the vector table and reset handler implementation
 * for initializing memory sections before calling the main application.
 */

#include <stdint.h>

// External symbols provided by the linker script
extern uint32_t _sidata; ///< Start of init values in flash (.data)
extern uint32_t _sdata;  ///< Start of .data in RAM
extern uint32_t _edata;  ///< End of .data in RAM
extern uint32_t _sbss;   ///< Start of .bss
extern uint32_t _ebss;   ///< End of .bss
extern int main(void);   ///< Main application entry point

// Stack top defined in the linker script
extern uint32_t _estack;

/**
 * @brief Default interrupt handler.
 *
 * Called if no specific interrupt handler is defined.
 * Loops forever to indicate an unhandled interrupt.
 */
void Default_Handler(void) {
    while (1);
}

#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))

void Reset_Handler(void);
void NMI_Handler(void)             WEAK_ALIAS;
void HardFault_Handler(void)       WEAK_ALIAS;
void MemManage_Handler(void)       WEAK_ALIAS;
void BusFault_Handler(void)        WEAK_ALIAS;
void UsageFault_Handler(void)      WEAK_ALIAS;
void SVC_Handler(void)             WEAK_ALIAS;
void DebugMon_Handler(void)        WEAK_ALIAS;
void PendSV_Handler(void)          WEAK_ALIAS;
void SysTick_Handler(void)         WEAK_ALIAS;

/**
 * @brief Vector table for Cortex-M4.
 *
 * This table contains the initial stack pointer and pointers
 * to all exception and interrupt handlers.
 */
__attribute__((section(".isr_vector")))
const void* vector_table[] = {
    &_estack,               ///< Initial Stack Pointer
    Reset_Handler,          ///< Reset Handler
    NMI_Handler,            ///< NMI Handler
    HardFault_Handler,      ///< Hard Fault Handler
    MemManage_Handler,      ///< MPU Fault Handler
    BusFault_Handler,       ///< Bus Fault Handler
    UsageFault_Handler,     ///< Usage Fault Handler
    0, 0, 0, 0,             ///< Reserved
    SVC_Handler,            ///< SVCall Handler
    DebugMon_Handler,       ///< Debug Monitor Handler
    0,                      ///< Reserved
    PendSV_Handler,         ///< PendSV Handler
    SysTick_Handler         ///< SysTick Handler
};

/**
 * @brief Reset handler for Cortex-M4.
 *
 * Initializes the .data and .bss sections, enables interrupts,
 * and calls the main application.
 */
void Reset_Handler(void) {
    // Copy .data section from flash to RAM
    uint32_t* src = &_sidata;
    uint32_t* dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Zero out the .bss section
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Enable interrupts globally (optional in some RTOS setups)
    __asm volatile ("cpsie i");

    // Call main
    main();

    // Loop forever if main returns
    while (1);
}
