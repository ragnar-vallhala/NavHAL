/*
 * Copyright (C) 2026 NAVRobotec Pvt Ltd
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
 * @file test_interrupt.c
 * @brief The AVR has no pending register; each flag lives in its peripheral.
 *
 * @details
 * These assert the three answers the port can give, because until recently it
 * gave one of them wrongly: is_pending returned a flat false and
 * clear_all_pending was an empty body that reported success.
 *
 * Timer1 is the instrument. The timebase owns Timer0 and hal_timer only
 * touches 1 and 2 when a caller starts one, so Timer1 is free here; every case
 * restores what it changed.
 */

#include "test_interrupt.h"
#include "navhal.h"
#include <avr/io.h>
#include <stdint.h>

/* Force a Timer1 overflow within two counts rather than waiting out 65536 of
 * them, and leave the timer as it was found. */
static void _overflow_timer1(void) {
  uint8_t a = TCCR1A, b = TCCR1B;
  TCCR1A = 0u;
  TCCR1B = (uint8_t)(1u << CS10); /* F_CPU, no prescale */
  TIFR1 = (uint8_t)(1u << TOV1);  /* drop any stale flag */
  TCNT1 = 0xFFFEu;
  uint16_t spin = 1000u;
  while (((TIFR1 & (1u << TOV1)) == 0u) && spin)
    spin--;
  TCCR1B = 0u; /* stop before restoring, so the restore cannot race */
  TCCR1A = a;
  TCCR1B = b;
}

void test_avr_is_pending_sees_a_latched_flag(void) {
  _overflow_timer1();
  TEST_ASSERT_TRUE(hal_interrupt_is_pending(HAL_IRQ_TIMER1_OVF));
  hal_interrupt_clear_pending(HAL_IRQ_TIMER1_OVF);
}

/* Reads TIFR1 rather than asking is_pending: a clear case that checks its work
 * through is_pending passes for free on a port where is_pending always says
 * "no", which is exactly the bug this suite exists for. */
void test_avr_clear_pending_clears_one_flag(void) {
  _overflow_timer1();
  TEST_ASSERT_BITS_HIGH(1u << TOV1, TIFR1);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_interrupt_clear_pending(HAL_IRQ_TIMER1_OVF));
  TEST_ASSERT_BITS_LOW(1u << TOV1, TIFR1);
}

/* TIFR1 holds nothing but write-1-to-clear flags, so clearing one must leave
 * its neighbours alone -- the reason the implementation writes the single bit
 * there instead of OR-ing it in. */
/* TIFR1 holds nothing but write-1-to-clear flags, so clearing one must leave
 * its neighbours standing -- the reason the implementation writes the single
 * bit there rather than OR-ing it in, which would clear every flag set. */
/* Not tested here: that clearing TOV1 leaves its neighbours in TIFR1 standing.
 * The flags are write-1-to-clear, so the implementation writes the single bit
 * rather than OR-ing it in -- but simavr assigns the byte instead of honouring
 * w1c, and clears every flag in the register, so the emulator cannot tell a
 * correct implementation from a careless one. NAVTEST_SKIP_ON_PIL is no help:
 * its probe is a Cortex DWT trick and returns false on AVR. Wants an
 * ATmega328P on the bench. */

void test_avr_clear_all_pending_clears_the_latched_flags(void) {
  _overflow_timer1();
  TEST_ASSERT_BITS_HIGH(1u << TOV1, TIFR1);
  hal_interrupt_clear_all_pending();
  TEST_ASSERT_BITS_LOW(1u << TOV1, TIFR1);
}

/* Readable but not clearable on its own: SPIF goes away on reading SPSR then
 * SPDR, so a generic clear must refuse rather than appear to work. */
void test_avr_clear_pending_refuses_a_side_effect_flag(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_interrupt_clear_pending(HAL_IRQ_SPI_STC));
}

/* EE_READY is a level condition with no latched flag at all. */
void test_avr_clear_pending_rejects_a_flagless_vector(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_interrupt_clear_pending(HAL_IRQ_EE_READY));
  TEST_ASSERT_TRUE(!hal_interrupt_is_pending(HAL_IRQ_EE_READY));
}

NAVTEST_CASE_DECL(test_avr_is_pending_sees_a_latched_flag);
NAVTEST_CASE_DECL(test_avr_clear_pending_clears_one_flag);
NAVTEST_CASE_DECL(test_avr_clear_all_pending_clears_the_latched_flags);
NAVTEST_CASE_DECL(test_avr_clear_pending_refuses_a_side_effect_flag);
NAVTEST_CASE_DECL(test_avr_clear_pending_rejects_a_flagless_vector);

static const navtest_case_t avr_interrupt_cases[] = {
    NAVTEST_CASE(test_avr_is_pending_sees_a_latched_flag),
    NAVTEST_CASE(test_avr_clear_pending_clears_one_flag),
    NAVTEST_CASE(test_avr_clear_all_pending_clears_the_latched_flags),
    NAVTEST_CASE(test_avr_clear_pending_refuses_a_side_effect_flag),
    NAVTEST_CASE(test_avr_clear_pending_rejects_a_flagless_vector),
};

const navtest_suite_t test_avr_interrupt_suite = {
    .name = "AVR INTERRUPT FLAGS",
    .cases = avr_interrupt_cases,
    .count = sizeof(avr_interrupt_cases) / sizeof(avr_interrupt_cases[0]),
    .between = NULL,
};
