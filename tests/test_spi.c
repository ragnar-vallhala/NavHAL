#include "test_spi.h"
#include "core/cortex-m4/spi.h"
#include "core/cortex-m4/spi_reg.h"
#include "navtest/navtest.h"
#include <stddef.h>
#include <stdint.h>

void test_spi_init_config(void) {
  hal_spi_config_t config = {.baudrate = SPI_BAUDRATE_DIV16,
                             .cpol = SPI_CPOL_HIGH,
                             .cpha = SPI_CPHA_2EDGE,
                             .datasize = SPI_DATASIZE_16BIT,
                             .firstbit = SPI_FIRSTBIT_LSB};
  hal_spi_init(SPI_1, &config);
  volatile SPI_Reg_Typedef *S1 = GET_SPIx_BASE(1);

  TEST_ASSERT_EQUAL_UINT32(SPI_BAUDRATE_DIV16 << SPI_CR1_BR_Pos,
                           S1->CR1 & SPI_CR1_BR_Msk);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_CPOL);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_CPHA);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_DFF);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_LSBFIRST);
  TEST_ASSERT_TRUE(S1->CR1 & SPI_CR1_MSTR);
}
