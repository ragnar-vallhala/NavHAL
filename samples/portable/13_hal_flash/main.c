
#define CORTEX_M4
#include "navhal.h"

hal_flash_record_t rec;

int main(void)
{
    hal_timebase_init(1000); /**< Initialize SysTick for delays */
    hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});   /**< Initialize HAL_UART_2 at 9600 baud */
    hal_uart_print(HAL_UART_2, "HAL Flash Sample Application\n");
    rec.crc = 123;
    hal_flash_save(1, &rec, sizeof(hal_flash_record_t));
    uint8_t size = sizeof(hal_flash_record_t);
    hal_flash_record_t buff;
    hal_flash_read(1, &buff, &size);

    hal_uart_print(HAL_UART_2, "rec CRC: ");
    hal_uart_print(HAL_UART_2, rec.crc);
    hal_uart_print(HAL_UART_2, " | Key: ");
    hal_uart_print(HAL_UART_2, rec.key);
    hal_uart_print(HAL_UART_2, " | Magic: ");
    hal_uart_print(HAL_UART_2, rec.magic);
    hal_uart_print(HAL_UART_2, " | Reserved: ");
    hal_uart_print(HAL_UART_2, rec.reserved);
    hal_uart_print(HAL_UART_2, " | Size: ");
    hal_uart_print(HAL_UART_2, rec.size);
    hal_uart_print(HAL_UART_2, " | Status: ");
    hal_uart_print(HAL_UART_2, rec.status);
    hal_uart_print(HAL_UART_2, " \n");

    hal_uart_print(HAL_UART_2, "buff CRC: ");
    hal_uart_print(HAL_UART_2, buff.crc);
    hal_uart_print(HAL_UART_2, " | Key: ");
    hal_uart_print(HAL_UART_2, buff.key);
    hal_uart_print(HAL_UART_2, " | Magic: ");
    hal_uart_print(HAL_UART_2, buff.magic);
    hal_uart_print(HAL_UART_2, " | Reserved: ");
    hal_uart_print(HAL_UART_2, buff.reserved);
    hal_uart_print(HAL_UART_2, " | Size: ");
    hal_uart_print(HAL_UART_2, buff.size);
    hal_uart_print(HAL_UART_2, " | Status: ");
    hal_uart_print(HAL_UART_2, buff.status);
    hal_uart_print(HAL_UART_2, " \n");

    hal_uart_print(HAL_UART_2, "Storage needs compaction: ");
    hal_uart_print(HAL_UART_2, hal_flash_needs_compaction() ? 1 : 0);
    hal_uart_print(HAL_UART_2, " \n");
}
