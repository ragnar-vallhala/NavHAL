/**
 * @file tests/test_dma.h
 * @brief DMA test function declarations for NavTest.
 */

#define CORTEX_M4
#ifndef TEST_DMA_H
#define TEST_DMA_H
#include "common/hal_config.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_DMA

void test_dma_clock_enable_dma1(void);
void test_dma_clock_enable_dma2(void);
void test_dma_init_sets_channel(void);
void test_dma_init_sets_direction_m2p(void);
void test_dma_init_sets_direction_p2m(void);
void test_dma_init_sets_minc(void);
void test_dma_init_sets_priority(void);
void test_dma_init_sets_ndtr(void);
void test_dma_init_sets_peripheral_address(void);
void test_dma_init_sets_memory_address(void);
void test_dma_init_stream_disabled_before_configure(void);
void test_dma_start_enables_stream(void);
void test_dma_stop_disables_stream(void);
void test_dma_transfer_complete_returns_zero_before_start(void);
void test_dma_clear_flags_clears_isr(void);
void test_hal_dma_init_rejects_null_config(void);
void test_hal_dma_start_rejects_null_config(void);
void test_hal_dma_stop_rejects_null_config(void);

extern const navtest_suite_t test_dma_suite;

#endif /* NAVHAL_HAS_DMA */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TEST_DMA_H */
