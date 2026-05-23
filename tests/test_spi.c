#include "test_spi.h"
#include "navhal_port_spi.h"
#include "family/spi_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stddef.h>
#include <stdint.h>

void test_spi_init_config(void) {
  /* Renode's SPI model doesn't reflect DFF / LSBFIRST writes back; this
   * specific test fails there. The CPOL-LOW / CPHA-1EDGE variant below
   * passes both. */
  NAVTEST_SKIP_ON_PIL();
  hal_spi_config_t config = {.baudrate = HAL_SPI_BAUDRATE_DIV16,
                             .cpol = HAL_SPI_CPOL_HIGH,
                             .cpha = HAL_SPI_CPHA_2EDGE,
                             .datasize = HAL_SPI_DATASIZE_16BIT,
                             .firstbit = HAL_SPI_FIRSTBIT_LSB};
  hal_spi_init(HAL_SPI_1, &config);
  volatile SPI_Reg_Typedef *S1 = GET_SPIx_BASE(1);

  TEST_ASSERT_EQUAL_UINT32(HAL_SPI_BAUDRATE_DIV16 << SPI_CR1_BR_Pos,
                           S1->CR1 & SPI_CR1_BR_Msk);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_CPOL);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_CPHA);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_DFF);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_LSBFIRST);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_MSTR);
}

/* -------------------- Additional coverage -------------------- */

void test_hal_spi_init_returns_ok(void) {
  hal_spi_config_t cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV16,
                          .cpol = HAL_SPI_CPOL_HIGH,
                          .cpha = HAL_SPI_CPHA_2EDGE,
                          .datasize = HAL_SPI_DATASIZE_8BIT,
                          .firstbit = HAL_SPI_FIRSTBIT_MSB};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_spi_init(HAL_SPI_1, &cfg));
}

void test_hal_spi_init_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_spi_init(HAL_SPI_1, NULL));
}

void test_hal_spi_transmit_rejects_null_data(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_spi_transmit(HAL_SPI_1, NULL, 4, 100));
}

void test_hal_spi_receive_rejects_null_data(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_spi_receive(HAL_SPI_1, NULL, 4, 100));
}

void test_hal_spi_transmit_receive_rejects_null_data(void) {
  uint8_t buf[4] = {0};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_spi_transmit_receive(HAL_SPI_1, NULL, buf, 4, 100));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_spi_transmit_receive(HAL_SPI_1, buf, NULL, 4, 100));
}

void test_hal_spi_init_cpol_low_cpha_1edge(void) {
  /* Verify the alternate edge/polarity combo also lands in CR1. */
  hal_spi_config_t cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV8,
                          .cpol = HAL_SPI_CPOL_LOW,
                          .cpha = HAL_SPI_CPHA_1EDGE,
                          .datasize = HAL_SPI_DATASIZE_8BIT,
                          .firstbit = HAL_SPI_FIRSTBIT_MSB};
  hal_spi_init(HAL_SPI_1, &cfg);
  volatile SPI_Reg_Typedef *S1 = GET_SPIx_BASE(1);
  TEST_ASSERT_FALSE(S1->CR1 & SPI_CR1_CPOL);
  TEST_ASSERT_FALSE(S1->CR1 & SPI_CR1_CPHA);
  TEST_ASSERT_FALSE(S1->CR1 & SPI_CR1_DFF);
  TEST_ASSERT_FALSE(S1->CR1 & SPI_CR1_LSBFIRST);
}

static const navtest_case_t spi_cases[] = {
    NAVTEST_CASE(test_spi_init_config),
    NAVTEST_CASE(test_hal_spi_init_returns_ok),
    NAVTEST_CASE(test_hal_spi_init_rejects_null_config),
    NAVTEST_CASE(test_hal_spi_transmit_rejects_null_data),
    NAVTEST_CASE(test_hal_spi_receive_rejects_null_data),
    NAVTEST_CASE(test_hal_spi_transmit_receive_rejects_null_data),
    NAVTEST_CASE(test_hal_spi_init_cpol_low_cpha_1edge),
};

const navtest_suite_t test_spi_suite = {
    .name = "SPI",
    .cases = spi_cases,
    .count = sizeof(spi_cases) / sizeof(spi_cases[0]),
    .between = NULL,
};
