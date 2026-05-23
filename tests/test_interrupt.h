/**
 * @file tests/test_interrupt.h
 * @brief Standardized hal_interrupt_* (NVIC) API tests.
 */

#ifndef TEST_INTERRUPT_H
#define TEST_INTERRUPT_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_hal_interrupt_enable_sets_iser_bit(void);
void test_hal_interrupt_disable_sets_icer_bit(void);
void test_hal_interrupt_clear_pending_clears_ispr_bit(void);
void test_hal_interrupt_set_get_priority_round_trip(void);
void test_hal_interrupt_is_pending_after_set(void);
void test_hal_interrupt_attach_then_dispatch_runs_callback(void);
void test_hal_interrupt_detach_clears_callback(void);
void test_hal_interrupt_disable_then_restore_global(void);
void test_hal_interrupt_clear_all_pending_zeros_icpr(void);

/* error paths */
void test_hal_interrupt_enable_rejects_negative_irq(void);
void test_hal_interrupt_disable_rejects_negative_irq(void);
void test_hal_interrupt_clear_pending_rejects_negative_irq(void);
void test_hal_interrupt_attach_rejects_out_of_range(void);
void test_hal_interrupt_detach_rejects_out_of_range(void);

extern const navtest_suite_t test_interrupt_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_INTERRUPT_H
