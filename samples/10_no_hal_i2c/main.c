#define CORTEX_M4
#include "navhal.h"
#include <stdint.h>

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
int wait_flag(volatile uint32_t *reg, uint32_t mask)
{
  int timeout = TIMEOUT;
  while (((*reg & mask) == 0) && --timeout)
  {
  }
  return (timeout > 0);
}

void uart2_write_hex(uint8_t val)
{
  const char hex_chars[] = "0123456789ABCDEF";
  uart2_write(hex_chars[(val >> 4) & 0xF]);
  uart2_write(hex_chars[val & 0xF]);
}

void uart2_write_uint16(uint16_t val)
{
  char buf[6] = {0};
  int i = 4;
  if (val == 0)
  {
    uart2_write('0');
    return;
  }
  while (val && i >= 0)
  {
    buf[i--] = (val % 10) + '0';
    val /= 10;
  }
  uart2_write(&buf[i + 1]);
}

void i2c1_init(void)
{
  // Enable clocks
  RCC_AHB1ENR |= (1 << 1);  // GPIOB
  RCC_APB1ENR |= (1 << 21); // I2C1

  // Configure PB8, PB9 as AF4 (I2C), open-drain, high speed, pull-up
  GPIOB_MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
  GPIOB_MODER |= ((2 << (8 * 2)) | (2 << (9 * 2))); // AF mode

  GPIOB_OTYPER |= (1 << 8) | (1 << 9);                // Open-drain
  GPIOB_OSPEEDR |= ((3 << (8 * 2)) | (3 << (9 * 2))); // High speed

  GPIOB_PUPDR &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
  GPIOB_PUPDR |= ((1 << (8 * 2)) | (1 << (9 * 2))); // Pull-up enabled internally (better with extern pull-ups)

  GPIOB_AFRH &= ~((0xF << ((8 - 8) * 4)) | (0xF << ((9 - 8) * 4)));
  GPIOB_AFRH |= ((4 << ((8 - 8) * 4)) | (4 << ((9 - 8) * 4))); // AF4

  // Reset I2C1
  I2C1_CR1 |= (1 << 15); // SW reset
  I2C1_CR1 &= ~(1 << 15);

  // Configure timing (assuming PCLK1 = 16 MHz)
  I2C1_CR2 = 16;   // Peripheral clock freq in MHz
  I2C1_CCR = 80;   // 100kHz standard mode: CCR = Fpclk1/(2*Fscl)
  I2C1_TRISE = 17; // TRISE = Fpclk1 + 1

  // Enable peripheral
  I2C1_CR1 |= (1 << 0); // PE
}

void i2c1_start(void)
{
  I2C1_CR1 |= (1 << 8); // START
  if (!wait_flag(&I2C1_SR1, 1 << 0))
  {
    uart2_write("I2C START timeout\n\r");
  }
}

void i2c1_stop(void)
{
  I2C1_CR1 |= (1 << 9); // STOP
}

void i2c1_write_addr(uint8_t addr)
{
  I2C1_DR = addr;
  if (!wait_flag(&I2C1_SR1, 1 << 1))
  { // ADDR flag
    uart2_write("I2C ADDR timeout\n\r");
  }
  (void)I2C1_SR2; // Clear ADDR flag
}

void i2c1_write_data(uint8_t data)
{
  if (!wait_flag(&I2C1_SR1, 1 << 7))
  { // TXE
    uart2_write("I2C TXE timeout\n\r");
  }
  I2C1_DR = data;
  if (!wait_flag(&I2C1_SR1, 1 << 2))
  { // BTF
    uart2_write("I2C BTF timeout\n\r");
  }
}

uint8_t i2c1_read_data_nack(void)
{
  I2C1_CR1 &= ~(1 << 10); // ACK = 0
  I2C1_CR1 |= (1 << 9);   // STOP
  if (!wait_flag(&I2C1_SR1, 1 << 6))
  { // RXNE
    uart2_write("I2C RXNE timeout\n\r");
  }
  return I2C1_DR;
}

uint8_t bmp180_read_reg(uint8_t reg)
{
  uint8_t val;
  i2c1_start();
  i2c1_write_addr(BMP180_ADDR << 1); // Write mode
  i2c1_write_data(reg);
  i2c1_start();
  i2c1_write_addr((BMP180_ADDR << 1) | 1); // Read mode

  // Enable ACK for first byte
  I2C1_CR1 |= (1 << 10); // ACK = 1
  if (!wait_flag(&I2C1_SR1, 1 << 6))
  {
    uart2_write("I2C RXNE timeout reg read\n\r");
  }
  val = I2C1_DR;
  // Disable ACK and STOP after last byte
  I2C1_CR1 &= ~(1 << 10); // ACK = 0
  I2C1_CR1 |= (1 << 9);   // STOP
  return val;
}

uint16_t bmp180_read_u16(uint8_t reg)
{
  uint16_t val;
  i2c1_start();
  i2c1_write_addr(BMP180_ADDR << 1);
  i2c1_write_data(reg);
  i2c1_start();
  i2c1_write_addr((BMP180_ADDR << 1) | 1);

  I2C1_CR1 |= (1 << 10); // ACK = 1
  if (!wait_flag(&I2C1_SR1, 1 << 6))
  {
    uart2_write("I2C RXNE timeout u16 high\n\r");
  }
  val = I2C1_DR << 8;

  I2C1_CR1 &= ~(1 << 10); // ACK = 0 (NACK next)
  I2C1_CR1 |= (1 << 9);   // STOP

  if (!wait_flag(&I2C1_SR1, 1 << 6))
  {
    uart2_write("I2C RXNE timeout u16 low\n\r");
  }
  val |= I2C1_DR;

  return val;
}

void bmp180_write_reg(uint8_t reg, uint8_t value)
{
  i2c1_start();
  i2c1_write_addr(BMP180_ADDR << 1); // Write mode
  i2c1_write_data(reg);
  i2c1_write_data(value);
  i2c1_stop();
}

int main(void)
{
  systick_init(1000);
  i2c1_init();
  uart2_init(9600);
  uart2_write("BMP180 Test Start\n\r");

  uint8_t chip_id = bmp180_read_reg(0xD0);
  uart2_write("Chip ID: 0x");
  uart2_write_hex(chip_id);
  uart2_write("\n\r");
  uint64_t i=0;
  while (1)
  {
    bmp180_write_reg(0xF4, 0x2E); // Start temperature measurement

    delay_ms(5);

    uint16_t raw_temp = bmp180_read_u16(0xF6);
    uart2_write(++i);
    uart2_write("\r\n");
    uart2_write(">Temp:");
    uart2_write_uint16(raw_temp);
    uart2_write("\r\n");
  }
}
