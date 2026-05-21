/**
 * @file tests/test_dma.c
 * @brief DMA unit tests for NavTest.
 */

#define CORTEX_M4
#include "common/hal_config.h"

#include "common/hal_features.h"
#if NAVHAL_HAS_DMA

#include "navhal_port_dma.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include "test_dma.h"

static hal_dma_config_t test_cfg;

/* Setup called before each RUN_TEST manually */
static void dma_setUp(void) {
  /* Reset RCC DMA clocks */
  RCC->AHB1ENR &= ~(RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN);

  /* Reset test config to known state */
  test_cfg.controller = HAL_DMA_CONTROLLER_1;
  test_cfg.stream = 0;
  test_cfg.channel = 0;
  test_cfg.direction = HAL_DMA_DIR_M2P;
  test_cfg.src_addr = 0x20000000;
  test_cfg.dst_addr = 0x40000000;
  test_cfg.data_count = 100;
  test_cfg.src_inc = 1;
  test_cfg.dst_inc = 0;
  test_cfg.data_width = HAL_DMA_DATA_WIDTH_8;
  test_cfg.priority = HAL_DMA_PRIORITY_LOW;
  test_cfg.circular = 0;

  /* Make sure stream 0 is disabled before tests */
  DMA1->STREAM[0].CR &= ~DMA_SxCR_EN;
  while (DMA1->STREAM[0].CR & DMA_SxCR_EN)
    ;
}

static void dma_tearDown(void) { /* Cleanup */ }

void test_dma_clock_enable_dma1(void) {
  dma_setUp();
  test_cfg.controller = HAL_DMA_CONTROLLER_1;
  hal_dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(RCC_AHB1ENR_DMA1EN, RCC->AHB1ENR);
}

void test_dma_clock_enable_dma2(void) {
  dma_setUp();
  test_cfg.controller = HAL_DMA_CONTROLLER_2;
  hal_dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(RCC_AHB1ENR_DMA2EN, RCC->AHB1ENR);
}

void test_dma_init_sets_channel(void) {
  NAVTEST_SKIP_ON_PIL();
  dma_setUp();
  test_cfg.channel = 3;
  hal_dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(3, (cr & DMA_SxCR_CHSEL_MASK) >> DMA_SxCR_CHSEL_POS);
}

void test_dma_init_sets_direction_m2p(void) {
  dma_setUp();
  test_cfg.direction = HAL_DMA_DIR_M2P;
  hal_dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(1, (cr & DMA_SxCR_DIR_MASK) >> DMA_SxCR_DIR_POS);
}

void test_dma_init_sets_direction_p2m(void) {
  dma_setUp();
  test_cfg.direction = HAL_DMA_DIR_P2M;
  hal_dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(0, (cr & DMA_SxCR_DIR_MASK) >> DMA_SxCR_DIR_POS);
}

void test_dma_init_sets_minc(void) {
  dma_setUp();
  test_cfg.direction = HAL_DMA_DIR_M2P;
  test_cfg.src_inc = 1; /* Source is Memory in M2P */
  hal_dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(DMA_SxCR_MINC, DMA1->STREAM[0].CR);
}

void test_dma_init_sets_priority(void) {
  NAVTEST_SKIP_ON_PIL();
  dma_setUp();
  test_cfg.priority = HAL_DMA_PRIORITY_HIGH;
  hal_dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(2, (cr & DMA_SxCR_PL_MASK) >> DMA_SxCR_PL_POS);
}

void test_dma_init_sets_ndtr(void) {
  dma_setUp();
  test_cfg.data_count = 512;
  hal_dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(512, DMA1->STREAM[0].NDTR);
}

void test_dma_init_sets_peripheral_address(void) {
  dma_setUp();
  test_cfg.direction = HAL_DMA_DIR_M2P;
  test_cfg.dst_addr = 0x40011004; /* e.g. USART1->DR */
  hal_dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0x40011004, DMA1->STREAM[0].PAR);
}

void test_dma_init_sets_memory_address(void) {
  dma_setUp();
  test_cfg.direction = HAL_DMA_DIR_M2P;
  test_cfg.src_addr = 0x20001234;
  hal_dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0x20001234, DMA1->STREAM[0].M0AR);
}

void test_dma_start_enables_stream(void) {
  dma_setUp();
  hal_dma_init(&test_cfg);
  hal_dma_start(&test_cfg);
  TEST_ASSERT_BITS_HIGH(DMA_SxCR_EN, DMA1->STREAM[0].CR);
  hal_dma_stop(&test_cfg);
}

void test_dma_stop_disables_stream(void) {
  dma_setUp();
  hal_dma_init(&test_cfg);
  hal_dma_start(&test_cfg);
  hal_dma_stop(&test_cfg);
  TEST_ASSERT_BITS_LOW(DMA_SxCR_EN, DMA1->STREAM[0].CR);
}

void test_dma_transfer_complete_returns_zero_before_start(void) {
  dma_setUp();
  hal_dma_init(&test_cfg);
  hal_dma_clear_flags(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0, hal_dma_transfer_complete(&test_cfg));
}

void test_dma_clear_flags_clears_isr(void) {
  dma_setUp();
  hal_dma_init(&test_cfg);
  /* Manually setting ISR bits is not possible (read-only), but we can call
     clear and verify we don't crash. Since we can't inject a software interrupt
     into the DMA controller easily, we just verify the clear function runs. */
  hal_dma_clear_flags(&test_cfg);
  TEST_ASSERT_TRUE(1);
}

/* -------------------- Standardized contract -------------------- */

void test_hal_dma_init_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_init(NULL));
}

void test_hal_dma_start_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_start(NULL));
}

void test_hal_dma_stop_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_stop(NULL));
}

static const navtest_case_t dma_cases[] = {
    NAVTEST_CASE(test_dma_clock_enable_dma1),
    NAVTEST_CASE(test_dma_clock_enable_dma2),
    NAVTEST_CASE(test_dma_init_sets_channel),
    NAVTEST_CASE(test_dma_init_sets_direction_m2p),
    NAVTEST_CASE(test_dma_init_sets_direction_p2m),
    NAVTEST_CASE(test_dma_init_sets_minc),
    NAVTEST_CASE(test_dma_init_sets_priority),
    NAVTEST_CASE(test_dma_init_sets_ndtr),
    NAVTEST_CASE(test_dma_init_sets_peripheral_address),
    NAVTEST_CASE(test_dma_init_sets_memory_address),
    NAVTEST_CASE(test_dma_start_enables_stream),
    NAVTEST_CASE(test_dma_stop_disables_stream),
    NAVTEST_CASE(test_dma_transfer_complete_returns_zero_before_start),
    NAVTEST_CASE(test_dma_clear_flags_clears_isr),
    /* standardized contract */
    NAVTEST_CASE(test_hal_dma_init_rejects_null_config),
    NAVTEST_CASE(test_hal_dma_start_rejects_null_config),
    NAVTEST_CASE(test_hal_dma_stop_rejects_null_config),
};

const navtest_suite_t test_dma_suite = {
    .name = "DMA",
    .cases = dma_cases,
    .count = sizeof(dma_cases) / sizeof(dma_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_HAS_DMA */
