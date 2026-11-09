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
 * @file src/arch/avr/interrupt/interrupt.c
 * @brief AVR / ATmega328P interrupt HAL backend.
 *
 * @details
 * Implements the @c hal_interrupt_* API (see
 * @c include/port/avr/navhal_port_interrupt.h) for the ATmega328P.
 *
 * The AVR has no NVIC: the vector table is flat, vector order fixes
 * priority, and there is no per-vector enable/pending register — each
 * peripheral owns its own interrupt-enable bit. This backend therefore
 * provides what *is* portable:
 *
 *  - global enable/disable via the SREG I-bit (`sei` / `cli`);
 *  - a callback table that @c hal_interrupt_dispatch routes through, so a
 *    driver ISR can hand an event to a registered ::hal_interrupt_callback_t.
 *
 * Operations with no AVR equivalent (per-IRQ enable/disable/pending) return
 * ::HAL_ERR_NOT_SUPPORTED; priority calls are accepted and ignored.
 */

#include "common/hal_interrupt.h"
#include "navhal_port_interrupt.h"
#include "internal/hal_interrupt_ops.h"
#include "internal/hal_interrupt_table.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>

/* The callback table lives in the common layer now; this port kept its own
 * copy of the same bounds check and indexed call. */

/** @brief True for an IRQ value that indexes a valid callback slot. */
static inline bool irq_in_range(hal_irq_t irq) {
  return irq >= HAL_IRQ_INT0 && irq < HAL_IRQ_COUNT;
}

/* ---- Per-IRQ control: not a thing on the AVR's flat vector table -------- */

/* Per-interrupt enable. There is no NVIC: each enable bit lives in the
 * peripheral that raises the interrupt, so this is the same shape as the flag
 * table below -- a register and a bit.
 *
 * `w1c` is the part that is easy to get wrong. Some enable bits share a
 * register with a flag that clears when 1 is written to it: ADIE sits beside
 * ADIF, ACIE beside ACI, TWIE beside TWINT, WDIE beside WDIF. A plain
 * `*reg |= mask` reads that flag back as 1 and writes it straight out again,
 * clearing a pending interrupt as a side effect of enabling one. So those bits
 * are masked to 0 on the way out, and enabling an interrupt never discards one.
 */
typedef struct {
  volatile uint8_t *reg;
  uint8_t mask;
  uint8_t w1c;
} avr_irq_mask_t;

/* In flash, not a switch: twenty-five cases of immediate loads overflowed the
 * 32K part by 860 bytes once the test image was linked. A row is four bytes and
 * the lookup is an index. */
typedef struct {
  uint16_t reg; /* SFR address; 0 means this line has no enable bit */
  uint8_t mask;
  uint8_t w1c;
} avr_irq_mask_row_t;

static const avr_irq_mask_row_t _irq_masks[HAL_IRQ_COUNT] PROGMEM = {
    [HAL_IRQ_INT0] = {(uint16_t)(uintptr_t)&EIMSK, 1u << INT0, 0},
    [HAL_IRQ_INT1] = {(uint16_t)(uintptr_t)&EIMSK, 1u << INT1, 0},
    [HAL_IRQ_PCINT0] = {(uint16_t)(uintptr_t)&PCICR, 1u << PCIE0, 0},
    [HAL_IRQ_PCINT1] = {(uint16_t)(uintptr_t)&PCICR, 1u << PCIE1, 0},
    [HAL_IRQ_PCINT2] = {(uint16_t)(uintptr_t)&PCICR, 1u << PCIE2, 0},
    [HAL_IRQ_WDT] = {(uint16_t)(uintptr_t)&WDTCSR, 1u << WDIE, 1u << WDIF},
    [HAL_IRQ_TIMER2_COMPA] = {(uint16_t)(uintptr_t)&TIMSK2, 1u << OCIE2A, 0},
    [HAL_IRQ_TIMER2_COMPB] = {(uint16_t)(uintptr_t)&TIMSK2, 1u << OCIE2B, 0},
    [HAL_IRQ_TIMER2_OVF] = {(uint16_t)(uintptr_t)&TIMSK2, 1u << TOIE2, 0},
    [HAL_IRQ_TIMER1_CAPT] = {(uint16_t)(uintptr_t)&TIMSK1, 1u << ICIE1, 0},
    [HAL_IRQ_TIMER1_COMPA] = {(uint16_t)(uintptr_t)&TIMSK1, 1u << OCIE1A, 0},
    [HAL_IRQ_TIMER1_COMPB] = {(uint16_t)(uintptr_t)&TIMSK1, 1u << OCIE1B, 0},
    [HAL_IRQ_TIMER1_OVF] = {(uint16_t)(uintptr_t)&TIMSK1, 1u << TOIE1, 0},
    [HAL_IRQ_TIMER0_COMPA] = {(uint16_t)(uintptr_t)&TIMSK0, 1u << OCIE0A, 0},
    [HAL_IRQ_TIMER0_COMPB] = {(uint16_t)(uintptr_t)&TIMSK0, 1u << OCIE0B, 0},
    [HAL_IRQ_TIMER0_OVF] = {(uint16_t)(uintptr_t)&TIMSK0, 1u << TOIE0, 0},
    [HAL_IRQ_SPI_STC] = {(uint16_t)(uintptr_t)&SPCR, 1u << SPIE, 0},
    [HAL_IRQ_USART_RX] = {(uint16_t)(uintptr_t)&UCSR0B, 1u << RXCIE0, 0},
    [HAL_IRQ_USART_UDRE] = {(uint16_t)(uintptr_t)&UCSR0B, 1u << UDRIE0, 0},
    [HAL_IRQ_USART_TX] = {(uint16_t)(uintptr_t)&UCSR0B, 1u << TXCIE0, 0},
    [HAL_IRQ_ADC] = {(uint16_t)(uintptr_t)&ADCSRA, 1u << ADIE, 1u << ADIF},
    [HAL_IRQ_EE_READY] = {(uint16_t)(uintptr_t)&EECR, 1u << EERIE, 0},
    [HAL_IRQ_ANALOG_COMP] = {(uint16_t)(uintptr_t)&ACSR, 1u << ACIE, 1u << ACI},
    [HAL_IRQ_TWI] = {(uint16_t)(uintptr_t)&TWCR, 1u << TWIE, 1u << TWINT},
    [HAL_IRQ_SPM_READY] = {(uint16_t)(uintptr_t)&SPMCSR, 1u << SPMIE, 0},
};

static bool _mask_of(hal_irq_t irq, avr_irq_mask_t *m) {
  if (!irq_in_range(irq))
    return false;
  uint16_t reg = pgm_read_word(&_irq_masks[irq].reg);
  if (reg == 0u)
    return false;
  m->reg = (volatile uint8_t *)(uintptr_t)reg;
  m->mask = pgm_read_byte(&_irq_masks[irq].mask);
  m->w1c = pgm_read_byte(&_irq_masks[irq].w1c);
  return true;
}

static hal_status_t avr_interrupt_enable(hal_irq_t irq) {
  avr_irq_mask_t m;
  if (!irq_in_range(irq) || !_mask_of(irq, &m))
    return HAL_ERR_INVALID_ARG;
  *m.reg = (uint8_t)((*m.reg & (uint8_t)~m.w1c) | m.mask);
  return HAL_OK;
}

static hal_status_t avr_interrupt_disable(hal_irq_t irq) {
  avr_irq_mask_t m;
  if (!irq_in_range(irq) || !_mask_of(irq, &m))
    return HAL_ERR_INVALID_ARG;
  *m.reg = (uint8_t)(*m.reg & (uint8_t)~(m.w1c | m.mask));
  return HAL_OK;
}

/* ---- Pending flags -----------------------------------------------------
 *
 * There is no central pending register: each vector's flag lives in its own
 * peripheral. Two kinds of home, and the difference decides how to clear one:
 *
 *   flag-only registers (EIFR, PCIFR, TIFR0/1/2) hold nothing but write-1-to-
 *     clear flags, so writing the single bit clears exactly that flag.
 *   mixed registers (ADCSRA, ACSR, WDTCSR, UCSR0A) hold control bits too, so
 *     the bit has to be OR'd in -- writing it alone would reconfigure or
 *     disable the peripheral.
 *
 * Several vectors have no flag that can be cleared this way at all. Their
 * flags clear as a side effect of reading or writing a data register -- USART
 * RXC0 on reading UDR0, SPIF on reading SPSR then SPDR -- and TWINT, which is
 * write-1-to-clear, also releases the TWI bus and starts the next operation,
 * which is not something a generic "clear pending" may do. Those report
 * HAL_ERR_NOT_SUPPORTED rather than pretending. */

typedef struct {
  volatile uint8_t *reg; /**< Where the flag lives, NULL if it has none. */
  uint8_t mask;          /**< The flag's bit. */
  bool flag_only;        /**< Register holds only w1c flags. */
  bool clearable;        /**< Clearable without a side effect. */
} avr_irq_flag_t;

/* Same shape and the same reason as _irq_masks: in flash, indexed, not a
 * switch. `flag_only` says every bit in the register is a flag, so a single-bit
 * write leaves the neighbours standing; `clearable` says the flag can be
 * cleared on its own terms at all -- TWINT releases the TWI bus when written,
 * and the USART and SPI flags clear by reading their data registers. */
#define AVR_FLAG_ONLY 0x01u
#define AVR_CLEARABLE 0x02u

typedef struct {
  uint16_t reg; /* SFR address; 0 means this line has no flag */
  uint8_t mask;
  uint8_t bits;
} avr_irq_flag_row_t;

static const avr_irq_flag_row_t _irq_flags[HAL_IRQ_COUNT] PROGMEM = {
    [HAL_IRQ_INT0] = {(uint16_t)(uintptr_t)&EIFR, 1u << INTF0, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_INT1] = {(uint16_t)(uintptr_t)&EIFR, 1u << INTF1, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_PCINT0] = {(uint16_t)(uintptr_t)&PCIFR, 1u << PCIF0, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_PCINT1] = {(uint16_t)(uintptr_t)&PCIFR, 1u << PCIF1, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_PCINT2] = {(uint16_t)(uintptr_t)&PCIFR, 1u << PCIF2, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_WDT] = {(uint16_t)(uintptr_t)&WDTCSR, 1u << WDIF, AVR_CLEARABLE},
    [HAL_IRQ_TIMER2_COMPA] = {(uint16_t)(uintptr_t)&TIFR2, 1u << OCF2A, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER2_COMPB] = {(uint16_t)(uintptr_t)&TIFR2, 1u << OCF2B, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER2_OVF] = {(uint16_t)(uintptr_t)&TIFR2, 1u << TOV2, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER1_CAPT] = {(uint16_t)(uintptr_t)&TIFR1, 1u << ICF1, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER1_COMPA] = {(uint16_t)(uintptr_t)&TIFR1, 1u << OCF1A, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER1_COMPB] = {(uint16_t)(uintptr_t)&TIFR1, 1u << OCF1B, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER1_OVF] = {(uint16_t)(uintptr_t)&TIFR1, 1u << TOV1, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER0_COMPA] = {(uint16_t)(uintptr_t)&TIFR0, 1u << OCF0A, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER0_COMPB] = {(uint16_t)(uintptr_t)&TIFR0, 1u << OCF0B, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_TIMER0_OVF] = {(uint16_t)(uintptr_t)&TIFR0, 1u << TOV0, AVR_FLAG_ONLY | AVR_CLEARABLE},
    [HAL_IRQ_ADC] = {(uint16_t)(uintptr_t)&ADCSRA, 1u << ADIF, AVR_CLEARABLE},
    [HAL_IRQ_ANALOG_COMP] = {(uint16_t)(uintptr_t)&ACSR, 1u << ACI, AVR_CLEARABLE},
    [HAL_IRQ_USART_TX] = {(uint16_t)(uintptr_t)&UCSR0A, 1u << TXC0, AVR_CLEARABLE},
    [HAL_IRQ_SPI_STC] = {(uint16_t)(uintptr_t)&SPSR, 1u << SPIF, 0},
    [HAL_IRQ_USART_RX] = {(uint16_t)(uintptr_t)&UCSR0A, 1u << RXC0, 0},
    [HAL_IRQ_USART_UDRE] = {(uint16_t)(uintptr_t)&UCSR0A, 1u << UDRE0, 0},
    [HAL_IRQ_TWI] = {(uint16_t)(uintptr_t)&TWCR, 1u << TWINT, 0},
};

static bool _flag_of(hal_irq_t irq, avr_irq_flag_t *f) {
  if (!irq_in_range(irq))
    return false;
  uint16_t reg = pgm_read_word(&_irq_flags[irq].reg);
  if (reg == 0u)
    return false;
  uint8_t bits = pgm_read_byte(&_irq_flags[irq].bits);
  f->reg = (volatile uint8_t *)(uintptr_t)reg;
  f->mask = pgm_read_byte(&_irq_flags[irq].mask);
  f->flag_only = (bits & AVR_FLAG_ONLY) != 0u;
  f->clearable = (bits & AVR_CLEARABLE) != 0u;
  return true;
}

static hal_status_t avr_interrupt_clear_pending(hal_irq_t irq) {
  avr_irq_flag_t f;
  if (!irq_in_range(irq) || !_flag_of(irq, &f))
    return HAL_ERR_INVALID_ARG;
  if (!f.clearable)
    return HAL_ERR_NOT_SUPPORTED;
  if (f.flag_only)
    *f.reg = f.mask; /* the other bits are flags too: writing 0 leaves them */
  else
    *f.reg |= f.mask; /* control bits share the register and must survive */
  return HAL_OK;
}

static bool avr_interrupt_is_pending(hal_irq_t irq) {
  avr_irq_flag_t f;
  if (!irq_in_range(irq) || !_flag_of(irq, &f))
    return false; /* no flag exists; not the same as "not pending", but a bool
                   * cannot say so and the vectors without one are level
                   * conditions that are never latched */
  return (*f.reg & f.mask) != 0u;
}

/* ---- Priority: the AVR has none; accept and ignore --------------------- */

/* The AVR fixes priority by vector position, so there is nothing to set -- but
 * a line that does not exist is still refused rather than accepted silently. */
static hal_status_t avr_interrupt_set_priority(hal_irq_t irq, uint8_t priority) {
  (void)priority;
  if (!irq_in_range(irq))
    return HAL_ERR_INVALID_ARG;
  return HAL_OK;
}

static uint8_t avr_interrupt_get_priority(hal_irq_t irq) {
  (void)irq;
  return 0;
}

/* ---- Callback table ---------------------------------------------------- */

static void avr_interrupt_dispatch(hal_irq_t irq) {
  /* Nothing to acknowledge: the AVR clears a vector's flag as it enters the
   * handler, and the flags that need clearing by hand are the ones no vector
   * was taken for. */
  (void)navhal_irq_invoke(irq);
}

/* SLEEP with the I-bit clear does not wake, so unlike Cortex-M's WFI this has
 * to enable interrupts across the instruction and put the caller's state back
 * afterwards. sei takes effect after the following instruction, which is what
 * makes sei+sleep atomic: no interrupt can land between them and leave the
 * core asleep with the wake condition already gone. A handler may therefore
 * run before this returns, where on Cortex-M it would stay pending. */
static void avr_cpu_idle(void) {
  uint8_t sreg = SREG;
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();
  SREG = sreg;
}

/* ---- Global interrupt enable (SREG I-bit) ------------------------------ */

static uint32_t avr_interrupt_disable_global(void) {
  uint8_t saved = SREG;
  cli();
  return saved;
}

static void avr_interrupt_enable_global(uint32_t state) {
  /* Restoring the whole SREG restores the I-bit to its prior value. */
  SREG = (uint8_t)state;
}

static void avr_interrupt_clear_all_pending(void) {
  /* No single register does it, so clear the flag-only registers, which is
   * every latched external and timer event. Deliberately not included: TWINT,
   * because clearing it releases the TWI bus mid-transfer; the USART and SPI
   * flags, which clear by touching their data registers; and ADIF/ACI/WDIF,
   * which share a register with control bits and so would need a
   * read-modify-write each -- hal_interrupt_clear_pending does those one at a
   * time, where the caller has asked for that one. */
  EIFR = (1u << INTF0) | (1u << INTF1);
  PCIFR = (1u << PCIF0) | (1u << PCIF1) | (1u << PCIF2);
  TIFR0 = (1u << OCF0A) | (1u << OCF0B) | (1u << TOV0);
  TIFR1 = (1u << ICF1) | (1u << OCF1A) | (1u << OCF1B) | (1u << TOV1);
  TIFR2 = (1u << OCF2A) | (1u << OCF2B) | (1u << TOV2);
}

/** @brief The AVR interrupt backend. */
const hal_interrupt_ops_t _hal_interrupt_ops = {
    .enable = avr_interrupt_enable,
    .disable = avr_interrupt_disable,
    .dispatch = avr_interrupt_dispatch,
    .cpu_idle = avr_cpu_idle,
    .disable_global = avr_interrupt_disable_global,
    .enable_global = avr_interrupt_enable_global,
    .set_priority = avr_interrupt_set_priority,
    .get_priority = avr_interrupt_get_priority,
    .is_pending = avr_interrupt_is_pending,
    .clear_pending = avr_interrupt_clear_pending,
    .clear_all_pending = avr_interrupt_clear_all_pending,
};
