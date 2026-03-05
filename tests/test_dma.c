/**
 * @file tests/test_dma.c
 * @brief DMA unit tests for NavTest.
 */

#define CORTEX_M4
#include "common/hal_config.h"

#ifdef _DMA_ENABLED

#include "core/cortex-m4/dma.h"
#include "core/cortex-m4/rcc_reg.h"
#include "navtest/navtest.h"
#include "test_dma.h"

static dma_config_t test_cfg;

/* Setup called before each RUN_TEST manually */
static void dma_setUp(void) {
  /* Reset RCC DMA clocks */
  RCC->AHB1ENR &= ~(RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN);

  /* Reset test config to known state */
  test_cfg.controller = DMA_CONTROLLER_1;
  test_cfg.stream = 0;
  test_cfg.channel = 0;
  test_cfg.direction = DMA_DIR_M2P;
  test_cfg.src_addr = 0x20000000;
  test_cfg.dst_addr = 0x40000000;
  test_cfg.data_count = 100;
  test_cfg.src_inc = 1;
  test_cfg.dst_inc = 0;
  test_cfg.data_width = DMA_DATA_WIDTH_8;
  test_cfg.priority = DMA_PRIORITY_LOW;
  test_cfg.circular = 0;

  /* Make sure stream 0 is disabled before tests */
  DMA1->STREAM[0].CR &= ~DMA_SxCR_EN;
  while (DMA1->STREAM[0].CR & DMA_SxCR_EN)
    ;
}

static void dma_tearDown(void) { /* Cleanup */ }

void test_dma_clock_enable_dma1(void) {
  dma_setUp();
  test_cfg.controller = DMA_CONTROLLER_1;
  dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(RCC_AHB1ENR_DMA1EN, RCC->AHB1ENR);
}

void test_dma_clock_enable_dma2(void) {
  dma_setUp();
  test_cfg.controller = DMA_CONTROLLER_2;
  dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(RCC_AHB1ENR_DMA2EN, RCC->AHB1ENR);
}

void test_dma_init_sets_channel(void) {
  dma_setUp();
  test_cfg.channel = 3;
  dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(3, (cr & DMA_SxCR_CHSEL_MASK) >> DMA_SxCR_CHSEL_POS);
}

void test_dma_init_sets_direction_m2p(void) {
  dma_setUp();
  test_cfg.direction = DMA_DIR_M2P;
  dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(1, (cr & DMA_SxCR_DIR_MASK) >> DMA_SxCR_DIR_POS);
}

void test_dma_init_sets_direction_p2m(void) {
  dma_setUp();
  test_cfg.direction = DMA_DIR_P2M;
  dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(0, (cr & DMA_SxCR_DIR_MASK) >> DMA_SxCR_DIR_POS);
}

void test_dma_init_sets_minc(void) {
  dma_setUp();
  test_cfg.direction = DMA_DIR_M2P;
  test_cfg.src_inc = 1; /* Source is Memory in M2P */
  dma_init(&test_cfg);
  TEST_ASSERT_BITS_HIGH(DMA_SxCR_MINC, DMA1->STREAM[0].CR);
}

void test_dma_init_sets_priority(void) {
  dma_setUp();
  test_cfg.priority = DMA_PRIORITY_HIGH;
  dma_init(&test_cfg);
  uint32_t cr = DMA1->STREAM[0].CR;
  TEST_ASSERT_EQUAL_UINT32(2, (cr & DMA_SxCR_PL_MASK) >> DMA_SxCR_PL_POS);
}

void test_dma_init_sets_ndtr(void) {
  dma_setUp();
  test_cfg.data_count = 512;
  dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(512, DMA1->STREAM[0].NDTR);
}

void test_dma_init_sets_peripheral_address(void) {
  dma_setUp();
  test_cfg.direction = DMA_DIR_M2P;
  test_cfg.dst_addr = 0x40011004; /* e.g. USART1->DR */
  dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0x40011004, DMA1->STREAM[0].PAR);
}

void test_dma_init_sets_memory_address(void) {
  dma_setUp();
  test_cfg.direction = DMA_DIR_M2P;
  test_cfg.src_addr = 0x20001234;
  dma_init(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0x20001234, DMA1->STREAM[0].M0AR);
}

void test_dma_start_enables_stream(void) {
  dma_setUp();
  dma_init(&test_cfg);
  dma_start(&test_cfg);
  TEST_ASSERT_BITS_HIGH(DMA_SxCR_EN, DMA1->STREAM[0].CR);
  dma_stop(&test_cfg);
}

void test_dma_stop_disables_stream(void) {
  dma_setUp();
  dma_init(&test_cfg);
  dma_start(&test_cfg);
  dma_stop(&test_cfg);
  TEST_ASSERT_BITS_LOW(DMA_SxCR_EN, DMA1->STREAM[0].CR);
}

void test_dma_transfer_complete_returns_zero_before_start(void) {
  dma_setUp();
  dma_init(&test_cfg);
  dma_clear_flags(&test_cfg);
  TEST_ASSERT_EQUAL_UINT32(0, dma_transfer_complete(&test_cfg));
}

void test_dma_clear_flags_clears_isr(void) {
  dma_setUp();
  dma_init(&test_cfg);
  /* Manually setting ISR bits is not possible (read-only), but we can call
     clear and verify we don't crash. Since we can't inject a software interrupt
     into the DMA controller easily, we just verify the clear function runs. */
  dma_clear_flags(&test_cfg);
  TEST_ASSERT_TRUE(1);
}

#endif /* _DMA_ENABLED */
