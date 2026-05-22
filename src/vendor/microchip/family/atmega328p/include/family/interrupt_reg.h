/**
 * @file family/interrupt_reg.h
 * @brief ATmega328P interrupt-vector identifiers.
 *
 * @details
 * Supplies ::hal_irq_t — the portable IRQ-identifier type the
 * `hal_interrupt_*` API takes — for the ATmega328P. Values are the
 * datasheet vector numbers (vector 0 is RESET and is not enumerated).
 *
 * Unlike the Cortex-M NVIC, the AVR has no per-vector enable/priority
 * registers: each peripheral owns its own interrupt-enable bit and the
 * vector table is flat. ::hal_irq_t therefore serves as a callback-table
 * index and a peripheral selector; the `hal_interrupt_*` backend
 * (src/arch/avr/) maps it onto `sei`/`cli` and per-peripheral bits, and
 * ignores the priority argument.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef AVR_ATMEGA328P_INTERRUPT_REG_H
#define AVR_ATMEGA328P_INTERRUPT_REG_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ATmega328P interrupt sources (datasheet vector numbers).
 */
typedef enum {
  HAL_IRQ_INT0 = 1,         /**< External interrupt request 0. */
  HAL_IRQ_INT1 = 2,         /**< External interrupt request 1. */
  HAL_IRQ_PCINT0 = 3,       /**< Pin-change interrupt 0 (port B). */
  HAL_IRQ_PCINT1 = 4,       /**< Pin-change interrupt 1 (port C). */
  HAL_IRQ_PCINT2 = 5,       /**< Pin-change interrupt 2 (port D). */
  HAL_IRQ_WDT = 6,          /**< Watchdog time-out. */
  HAL_IRQ_TIMER2_COMPA = 7, /**< Timer2 compare match A. */
  HAL_IRQ_TIMER2_COMPB = 8, /**< Timer2 compare match B. */
  HAL_IRQ_TIMER2_OVF = 9,   /**< Timer2 overflow. */
  HAL_IRQ_TIMER1_CAPT = 10, /**< Timer1 input capture. */
  HAL_IRQ_TIMER1_COMPA = 11,/**< Timer1 compare match A. */
  HAL_IRQ_TIMER1_COMPB = 12,/**< Timer1 compare match B. */
  HAL_IRQ_TIMER1_OVF = 13,  /**< Timer1 overflow. */
  HAL_IRQ_TIMER0_COMPA = 14,/**< Timer0 compare match A. */
  HAL_IRQ_TIMER0_COMPB = 15,/**< Timer0 compare match B. */
  HAL_IRQ_TIMER0_OVF = 16,  /**< Timer0 overflow. */
  HAL_IRQ_SPI_STC = 17,     /**< SPI serial transfer complete. */
  HAL_IRQ_USART_RX = 18,    /**< USART receive complete. */
  HAL_IRQ_USART_UDRE = 19,  /**< USART data register empty. */
  HAL_IRQ_USART_TX = 20,    /**< USART transmit complete. */
  HAL_IRQ_ADC = 21,         /**< ADC conversion complete. */
  HAL_IRQ_EE_READY = 22,    /**< EEPROM ready. */
  HAL_IRQ_ANALOG_COMP = 23, /**< Analog comparator. */
  HAL_IRQ_TWI = 24,         /**< Two-wire (I²C) interface. */
  HAL_IRQ_SPM_READY = 25,   /**< Store-program-memory ready. */
} hal_irq_t;

/** @brief Count of enumerated ATmega328P interrupt vectors (excludes RESET). */
#define HAL_IRQ_COUNT 26

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* AVR_ATMEGA328P_INTERRUPT_REG_H */
