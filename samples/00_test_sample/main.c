/**
 * @file main.c
 * @brief BMX160 (ACC+GYR) + BMM150 (MAG via AUX IF) initialization and raw data read.
 */

#define CORTEX_M4
#include "navhal.h"

// BMX160 I2C 7-bit address
#define BMX160_ADDR 0x68

// BMX160 register map
#define BMX160_REG_CHIP_ID 0x00
#define BMX160_REG_ERR     0x02
#define BMX160_REG_STATUS  0x1B
#define BMX160_REG_MAG_DATA 0x04
#define BMX160_REG_GYR_DATA 0x0C
#define BMX160_REG_ACC_DATA 0x12
#define BMX160_REG_CMD     0x7E

// BMX160 commands
#define BMX160_CMD_ACC_NORMAL 0x11
#define BMX160_CMD_GYR_NORMAL 0x15
#define BMX160_CMD_MAG_NORMAL 0x19
#define BMX160_CMD_MAG_LP     0x1A   // MAG_IF low power

#define I2C_BUS I2C1

// ---------------- Low-level helpers ----------------
static void bmx160_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = {reg, val};
    hal_i2c_write(I2C_BUS, BMX160_ADDR, tx, 2);
}

static uint8_t bmx160_read_u8(uint8_t reg)
{
    uint8_t val = 0;
    hal_i2c_write_read(I2C_BUS, BMX160_ADDR, &reg, 1, &val, 1);
    return val;
}

static void bmm150_init_via_bmx160(void)
{
    // --- Put MAG_IF into setup mode ---
    bmx160_write(0x4C, 0x80); // MAG_IF[0], manual setup
    delay_ms(2);

    // --- Indirect write 0x01 to BMM150 reg 0x4B (power control: sleep mode) ---
    bmx160_write(0x4F, 0x01); // data
    bmx160_write(0x4E, 0x4B); // target register
    delay_ms(2);

    // --- REPXY = 0x01 (low power preset) ---
    bmx160_write(0x4F, 0x01);
    bmx160_write(0x4E, 0x51);

    // --- REPZ = 0x02 (low power preset) ---
    bmx160_write(0x4F, 0x02);
    bmx160_write(0x4E, 0x52);

    // --- Prepare MAG_IF[1–3] for data mode ---
    bmx160_write(0x4F, 0x02);
    bmx160_write(0x4E, 0x4C);
    bmx160_write(0x4D, 0x42);

    // --- Set ODR = 12.5Hz (mag_conf reg 0x44) ---
    bmx160_write(0x44, 0x05);

    // --- Switch to Data mode ---
    bmx160_write(0x4C, 0x00);

    // --- Put MAG_IF into low power mode (required) ---
    bmx160_write(0x7E, BMX160_CMD_MAG_LP);
    delay_ms(10);
}

static void bmx160_enable_sensors(void)
{
    // Accelerometer
    bmx160_write(BMX160_REG_CMD, BMX160_CMD_ACC_NORMAL);
    delay_ms(5);

    // Gyroscope
    bmx160_write(BMX160_REG_CMD, BMX160_CMD_GYR_NORMAL);
    delay_ms(80);

    // Magnetometer (via AUX IF)
    bmx160_write(BMX160_REG_CMD, BMX160_CMD_MAG_NORMAL);
    delay_ms(10);

    // Init BMM150 through AUX IF
    bmm150_init_via_bmx160();
}

// ========== MAIN ==========
int main(void)
{
    systick_init(1000);
    uart2_init(115200);

    // I2C config
    hal_i2c_config_t i2c_config = {
        .clock_speed = STANDARD_MODE,
        .own_address = I2C_MASTER,
        .acknowledge = true};

    // GPIO for I2C1 (PB8=SCL, PB9=SDA)
    hal_gpio_set_alternate_function(GPIO_PB08, GPIO_FUNC_I2C);
    hal_gpio_set_alternate_function(GPIO_PB09, GPIO_FUNC_I2C);
    hal_gpio_set_output_type(GPIO_PB08, GPIO_OPEN_DRAIN);
    hal_gpio_set_output_type(GPIO_PB09, GPIO_OPEN_DRAIN);
    hal_gpio_set_output_speed(GPIO_PB08, GPIO_VERY_HIGH_SPEED);
    hal_gpio_set_output_speed(GPIO_PB09, GPIO_VERY_HIGH_SPEED);

    if (hal_i2c_init(I2C_BUS, &i2c_config) != HAL_I2C_OK)
    {
        uart2_write("I2C init failed!\n");
        while (1);
    }

    // Check chip ID
    uint8_t chip_id = bmx160_read_u8(BMX160_REG_CHIP_ID);
    uart2_write("BMX160 CHIP_ID: ");
    uart2_write(chip_id);
    uart2_write("\n");

    // Enable sensors
    bmx160_enable_sensors();

    uint8_t tx[1], rx[12];

    while (1)
    {
        // --- Gyroscope ---
        tx[0] = BMX160_REG_GYR_DATA;
        if (hal_i2c_write_read(I2C_BUS, BMX160_ADDR, tx, 1, rx, 6) == HAL_I2C_OK)
        {
            int16_t gx = (rx[1] << 8) | rx[0];
            int16_t gy = (rx[3] << 8) | rx[2];
            int16_t gz = (rx[5] << 8) | rx[4];
            uart2_write("GYR: ");
            uart2_write_int(gx); uart2_write(",");
            uart2_write_int(gy); uart2_write(",");
            uart2_write_int(gz); uart2_write("\n");
        }

        // --- Accelerometer ---
        tx[0] = BMX160_REG_ACC_DATA;
        if (hal_i2c_write_read(I2C_BUS, BMX160_ADDR, tx, 1, rx, 6) == HAL_I2C_OK)
        {
            int16_t ax = (rx[1] << 8) | rx[0];
            int16_t ay = (rx[3] << 8) | rx[2];
            int16_t az = (rx[5] << 8) | rx[4];
            uart2_write("ACC: ");
            uart2_write_int(ax); uart2_write(",");
            uart2_write_int(ay); uart2_write(",");
            uart2_write_int(az); uart2_write("\n");
        }

        // --- Magnetometer ---
        tx[0] = BMX160_REG_MAG_DATA;
        if (hal_i2c_write_read(I2C_BUS, BMX160_ADDR, tx, 1, rx, 8) == HAL_I2C_OK)
        {
            int16_t mx = (rx[1] << 8) | rx[0];
            int16_t my = (rx[3] << 8) | rx[2];
            int16_t mz = (rx[5] << 8) | rx[4];
            uint16_t rhall = (rx[7] << 8) | rx[6];

            uart2_write("MAG: ");
            uart2_write_int(mx); uart2_write(",");
            uart2_write_int(my); uart2_write(",");
            uart2_write_int(mz);
            uart2_write(" RHALL:");
            uart2_write_int(rhall);
            uart2_write("\n");
        }

        // --- Status debug ---
        uint8_t st = bmx160_read_u8(BMX160_REG_STATUS);
        uart2_write("STATUS: ");
        uart2_write(st);
        uart2_write("\n");

        delay_ms(500);
    }
}
