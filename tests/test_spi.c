#include "test_spi.h"
#include "core/cortex-m4/spi.h"
#include "core/cortex-m4/spi_reg.h"
#include "navtest/navtest.h"
#include <stddef.h>
#include <stdint.h>

void test_spi_init_config(void) {
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
