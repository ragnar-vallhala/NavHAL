#include "test_i2c.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/i2c.h"
#include "core/cortex-m4/i2c_reg.h"
#include "navtest/navtest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void test_i2c_init_config(void) {
  hal_i2c_config_t config = {
      .clock_speed = STANDARD_MODE, .own_address = 0, .acknowledge = true};
  hal_i2c_init(I2C1, &config);
  I2C_Reg_Typedef *I2C = I2C_GET_BASE(I2C1);

  uint32_t pclk1 = hal_clock_get_apb1clk();
  TEST_ASSERT_EQUAL_UINT32(pclk1 / 1000000, I2C->CR2 & I2C_CR2_FREQ_MASK);

  // Standard mode CCR = pclk / (2 * 100kHz)
  uint32_t expected_ccr = pclk1 / 200000;
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, I2C->CCR & I2C_CCR_CCR_MASK);
  TEST_ASSERT_FALSE(I2C->CCR & I2C_CCR_FS_MASK);
}

void test_i2c_fast_mode_config(void) {
  hal_i2c_config_t config = {
      .clock_speed = FAST_MODE, .own_address = 0, .acknowledge = true};
  // Use I2C2 to avoid re-init error in current HAL
  hal_i2c_init(I2C2, &config);
  I2C_Reg_Typedef *I2C = I2C_GET_BASE(I2C2);

  uint32_t pclk1 = hal_clock_get_apb1clk();
  // Fast mode CCR = pclk / (3 * 400kHz)
  uint32_t expected_ccr = pclk1 / 1200000;
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, I2C->CCR & I2C_CCR_CCR_MASK);
  TEST_ASSERT_TRUE(I2C->CCR & I2C_CCR_FS_MASK);
}
