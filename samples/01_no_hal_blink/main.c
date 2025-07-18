/**
 * @file blink_led.c
 * @brief Blink an LED on STM32 using direct register access (no HAL/LL).
 * 
 * This program blinks the onboard LED connected to GPIOA pin 5 on STM32
 * (e.g., STM32F4xx series), using direct memory-mapped register manipulation.
 */

 #include <stdint.h>  ///< Include for uint32_t, optional if using std types

 /** 
  * @brief RCC AHB1 peripheral clock enable register
  * 
  * Address: 0x40023830  
  * Bit 0: GPIOAEN - Enable clock for GPIOA
  */
 #define RCC_AHB1ENR    (*(volatile uint32_t*)0x40023830)
 
 /**
  * @brief GPIOA mode register
  * 
  * Address: 0x40020000  
  * MODER5 (bits 11:10) control mode of PA5
  */
 #define GPIOA_MODER    (*(volatile uint32_t*)0x40020000)
 
 /**
  * @brief GPIOA output data register
  * 
  * Address: 0x40020014  
  * Bit 5 controls the output level of PA5
  */
 #define GPIOA_ODR      (*(volatile uint32_t*)0x40020014)
 
 /**
  * @brief Simple delay loop
  * 
  * A crude delay function using a for loop. Not precise; use timers for accurate delays.
  */
 void delay(void) {
     for (volatile int i = 0; i < 500000; i++);
 }
 
 /**
  * @brief Main entry point
  * 
  * Initializes GPIOA pin 5 as output and toggles it in an infinite loop.
  * Typically connected to an LED (e.g., onboard LED on Nucleo boards).
  */
 int main(void) {
     // 1. Enable GPIOA clock (bit 0)
     RCC_AHB1ENR |= (1 << 0);
 
     // 2. Set PA5 as output (MODER5 = 01)
     GPIOA_MODER &= ~(0x3 << (5 * 2));  // Clear mode bits for pin 5
     GPIOA_MODER |=  (0x1 << (5 * 2));  // Set mode to general purpose output
 
     // 3. Toggle PA5 in a loop
     while (1) {
         GPIOA_ODR ^= (1 << 5);  // Toggle PA5
         delay();
     }
 }
 