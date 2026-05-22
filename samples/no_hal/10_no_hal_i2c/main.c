#define CORTEX_M4
#include "navhal.h"
#include <stdint.h>

#define I2C_BUS I2C1
#define I2C_MODE STANDARD_MODE
#define I2C_PIN_1 GPIO_PB08
#define I2C_PIN_2 GPIO_PB09
#define BMX160_I2C_ADDR 0x68

#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000000UL)

#define RCC_BASE (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400UL)
#define I2C1_BASE (APB1PERIPH_BASE + 0x5400UL)

#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define GPIOB_MODER (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_AFRH (*(volatile uint32_t *)(GPIOB_BASE + 0x24))

#define I2C1_CR1 (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2 (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_OAR1 (*(volatile uint32_t *)(I2C1_BASE + 0x08))
#define I2C1_DR (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_SR1 (*(volatile uint32_t *)(I2C1_BASE + 0x14))
#define I2C1_SR2 (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C1_CCR (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE (*(volatile uint32_t *)(I2C1_BASE + 0x20))

#define BMP180_ADDR 0x77

#define TIMEOUT 1000000

// Timeout helper: returns 1 if flag set, 0 if timeout
int wait_flag(volatile uint32_t *reg, uint32_t mask) {
  int timeout = TIMEOUT;
  while (((*reg & mask) == 0) && --timeout) {
  }
  return (timeout > 0);
}

void uart2_write_hex(uint8_t val) {
  const char hex_chars[] = "0123456789ABCDEF";
  uart2_write(hex_chars[(val >> 4) & 0xF]);
  uart2_write(hex_chars[val & 0xF]);
}

void uart2_write_uint16(uint16_t val) {
  char buf[6] = {0};
  int i = 4;
  if (val == 0) {
    uart2_write('0');
    return;
  }
  while (val && i >= 0) {
    buf[i--] = (val % 10) + '0';
    val /= 10;
  }
  uart2_write(&buf[i + 1]);
}

void i2c1_init(void) {
  // Enable clocks for GPIOB
  RCC_AHB1ENR |= (1 << 1); // GPIOB

  // 1. Manually toggle SCL (PB8) to unstick any stuck I2C slave
  GPIOB_MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
  GPIOB_MODER |= ((1 << (8 * 2)) | (1 << (9 * 2))); // Output mode for PB8, PB9
  GPIOB_OTYPER |= (1 << 8) | (1 << 9);              // Open-drain
  GPIOB_OSPEEDR |= ((3 << (8 * 2)) | (3 << (9 * 2))); // High speed
  GPIOB_PUPDR &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
  GPIOB_PUPDR |= ((1 << (8 * 2)) | (1 << (9 * 2))); // Pull-up

  // Set SDA high
  (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) = (1 << 9); // BSRR, set PB9 high
  for (volatile int i = 0; i < 100; i++)
    ;

  // Toggle SCL 9 times to clear slaves waiting for clock
  for (int i = 0; i < 9; ++i) {
    (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) =
        (1 << (8 + 16)); // BSRR, clear PB8 low
    for (volatile int j = 0; j < 200; j++)
      ;
    (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) =
        (1 << 8); // BSRR, set PB8 high
    for (volatile int j = 0; j < 200; j++)
      ;
  }

  // Stop condition: SCL high, then SDA goes high
  (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) =
      (1 << (9 + 16)); // BSRR, clear PB9 low
  for (volatile int j = 0; j < 200; j++)
    ;
  (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) = (1 << 8); // BSRR, set PB8 high
  for (volatile int j = 0; j < 200; j++)
    ;
  (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) = (1 << 9); // BSRR, set PB9 high
  for (volatile int j = 0; j < 200; j++)
    ;

  // 2. Configure PB8, PB9 as AF4 (I2C)
  GPIOB_MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
  GPIOB_MODER |= ((2 << (8 * 2)) | (2 << (9 * 2))); // AF mode

  GPIOB_AFRH &= ~((0xF << ((8 - 8) * 4)) | (0xF << ((9 - 8) * 4)));
  GPIOB_AFRH |= ((4 << ((8 - 8) * 4)) | (4 << ((9 - 8) * 4))); // AF4

  // Enable I2C1 clock
  RCC_APB1ENR |= (1 << 21); // I2C1

  // Reset I2C1
  I2C1_CR1 |= (1 << 15); // SW reset
  for (volatile int j = 0; j < 1000; j++)
    ;
  I2C1_CR1 &= ~(1 << 15);

  // Configure timing (assuming PCLK1 = 16 MHz)
  I2C1_CR2 = 16;   // Peripheral clock freq in MHz
  I2C1_CCR = 80;   // 100kHz standard mode: CCR = Fpclk1/(2*Fscl)
  I2C1_TRISE = 17; // TRISE = Fpclk1 + 1

  // Enable peripheral
  I2C1_CR1 |= (1 << 0); // PE
}

uint8_t i2c1_read_regs(uint8_t dev_addr, uint8_t reg, uint8_t *data,
                       uint16_t len) {
  int timeout = TIMEOUT;
  while ((I2C1_SR2 & (1 << 1)) && --timeout)
    ;
  if (timeout <= 0) {
    uart2_write("I2C_R: BUS BUSY\n\r");
    return 0;
  }

  I2C1_CR1 |= (1 << 8); // START
  if (!wait_flag(&I2C1_SR1, 1 << 0)) {
    uart2_write("I2C_R: START TIMEOUT\n\r");
    return 0; // Wait SB
  }

  I2C1_DR = dev_addr << 1; // Write mode
  if (!wait_flag(&I2C1_SR1, 1 << 1)) {
    uart2_write("I2C_R: ADDR1 TIMEOUT\n\r");
    return 0; // Wait ADDR
  }
  (void)I2C1_SR1;
  (void)I2C1_SR2; // Clear ADDR

  if (!wait_flag(&I2C1_SR1, 1 << 7)) {
    uart2_write("I2C_R: TXE TIMEOUT\n\r");
    return 0; // Wait TXE
  }
  I2C1_DR = reg;
  if (!wait_flag(&I2C1_SR1, 1 << 2)) {
    uart2_write("I2C_R: BTF TIMEOUT\n\r");
    return 0; // Wait BTF
  }

  I2C1_CR1 |= (1 << 8); // START
  if (!wait_flag(&I2C1_SR1, 1 << 0)) {
    uart2_write("I2C_R: RESTART TIMEOUT\n\r");
    return 0; // Wait SB
  }

  I2C1_DR = (dev_addr << 1) | 1; // Read mode
  if (!wait_flag(&I2C1_SR1, 1 << 1)) {
    uart2_write("I2C_R: ADDR2 TIMEOUT\n\r");
    return 0; // Wait ADDR
  }

  if (len == 1) {
    I2C1_CR1 &= ~(1 << 10); // Clear ACK
    (void)I2C1_SR1;
    (void)I2C1_SR2;       // Clear ADDR
    I2C1_CR1 |= (1 << 9); // Set STOP

    if (!wait_flag(&I2C1_SR1, 1 << 6)) {
      uart2_write("I2C_R: RXNE TIMEOUT\n\r");
      return 0; // Wait RXNE
    }
    data[0] = I2C1_DR;
  } else {
    (void)I2C1_SR1;
    (void)I2C1_SR2;        // Clear ADDR
    I2C1_CR1 |= (1 << 10); // Set ACK

    for (uint16_t i = 0; i < len; i++) {
      if (i == len - 1) {
        I2C1_CR1 &= ~(1 << 10); // Clear ACK
        I2C1_CR1 |= (1 << 9);   // Set STOP
      }
      if (!wait_flag(&I2C1_SR1, 1 << 6)) {
        uart2_write("I2C_R: RXNE_N TIMEOUT\n\r");
        return 0; // Wait RXNE
      }
      data[i] = I2C1_DR;
    }
  }
  return 1;
}

uint8_t i2c1_write_regs(uint8_t dev_addr, uint8_t reg, uint8_t *data,
                        uint16_t len) {
  int timeout = TIMEOUT;
  while ((I2C1_SR2 & (1 << 1)) && --timeout)
    ;
  if (timeout <= 0) {
    uart2_write("I2C_W: BUS BUSY\n\r");
    return 0;
  }

  I2C1_CR1 |= (1 << 8); // START
  if (!wait_flag(&I2C1_SR1, 1 << 0)) {
    uart2_write("I2C_W: START TIMEOUT\n\r");
    return 0; // Wait SB
  }

  I2C1_DR = dev_addr << 1; // Write mode
  if (!wait_flag(&I2C1_SR1, 1 << 1)) {
    uart2_write("I2C_W: ADDR TIMEOUT\n\r");
    return 0; // Wait ADDR
  }
  (void)I2C1_SR1;
  (void)I2C1_SR2; // Clear ADDR

  if (!wait_flag(&I2C1_SR1, 1 << 7)) {
    uart2_write("I2C_W: TXE1 TIMEOUT\n\r");
    return 0; // Wait TXE
  }
  I2C1_DR = reg;

  for (uint16_t i = 0; i < len; i++) {
    if (!wait_flag(&I2C1_SR1, 1 << 7)) {
      uart2_write("I2C_W: TXE_N TIMEOUT\n\r");
      return 0; // Wait TXE
    }
    I2C1_DR = data[i];
  }

  if (!wait_flag(&I2C1_SR1, 1 << 2)) {
    uart2_write("I2C_W: BTF TIMEOUT\n\r");
    return 0; // Wait BTF
  }
  I2C1_CR1 |= (1 << 9); // STOP
  return 1;
}

uint8_t bmx160_read_reg(uint8_t dev_addr, uint8_t reg) {
  uint8_t val = 0;
  i2c1_read_regs(dev_addr, reg, &val, 1);
  return val;
}

void bmx160_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t value) {
  i2c1_write_regs(dev_addr, reg, &value, 1);
}

void uart2_write_int16(int16_t val) {
  if (val < 0) {
    uart2_write('-');
    val = -val;
  }
  uart2_write_uint16((uint16_t)val);
}

int main(void) {
  systick_init(1000);
  i2c1_init();
  uart2_init(9600);
  delay_ms(100);
  uart2_write("BMX160 Test Start\n\r");

  uint8_t addr = 0x68;
  uint8_t chip_id = bmx160_read_reg(addr, 0x00);
  if (chip_id != 0xD8) {
    addr = 0x69;
    chip_id = bmx160_read_reg(addr, 0x00);
  }

  uart2_write("BMX160 Chip ID: 0x");
  uart2_write_hex(chip_id);
  uart2_write("\n\r");

  // Power up accelerometer (CMD register 0x7E, value 0x11 for standard acc
  // mode)
  bmx160_write_reg(addr, 0x7E, 0x11);
  delay_ms(50); // wait for power up

  uint64_t i = 0;
  while (1) {
    uint8_t acc_data[6] = {0};
    if (i2c1_read_regs(addr, 0x12, acc_data, 6)) {
      int16_t ax = (int16_t)((acc_data[1] << 8) | acc_data[0]);
      int16_t ay = (int16_t)((acc_data[3] << 8) | acc_data[2]);
      int16_t az = (int16_t)((acc_data[5] << 8) | acc_data[4]);

      uart2_write("Loop ");
      uart2_write_uint16(++i);
      uart2_write("\n\r");

      uart2_write("AX:");
      uart2_write_int16(ax);
      uart2_write(" AY:");
      uart2_write_int16(ay);
      uart2_write(" AZ:");
      uart2_write_int16(az);
      uart2_write("\n\r");
    } else {
      uart2_write("I2C Read Error\n\r");
    }

    delay_ms(500);
  }
}
