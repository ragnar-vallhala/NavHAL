
#define CORTEX_M4
#include "navhal.h"

FlashRecord_t rec;

int main(void)
{
    systick_init(1000); /**< Initialize SysTick for delays */
    uart2_init(9600);   /**< Initialize UART2 at 9600 baud */

    _flash_erase_sector_(5); /**< Erase flash memory block 5 */

    rec.crc = 123;
    save_data_to_flash(1, &rec, sizeof(FlashRecord_t));
    uint8_t size = sizeof(FlashRecord_t);
    FlashRecord_t buff;
    read_data_from_flash(1, &buff, &size);

    uart2_write("rec CRC: ");
    uart2_write(rec.crc);
    uart2_write(" | Key: ");
    uart2_write(rec.key);
    uart2_write(" | Magic: ");
    uart2_write(rec.magic);
    uart2_write(" | Reserved: ");
    uart2_write(rec.reserved);
    uart2_write(" | Size: ");
    uart2_write(rec.size);
    uart2_write(" | Status: ");
    uart2_write(rec.status);
    uart2_write(" \n");

    uart2_write("buff CRC: ");
    uart2_write(buff.crc);
    uart2_write(" | Key: ");
    uart2_write(buff.key);
    uart2_write(" | Magic: ");
    uart2_write(buff.magic);
    uart2_write(" | Reserved: ");
    uart2_write(buff.reserved);
    uart2_write(" | Size: ");
    uart2_write(buff.size);
    uart2_write(" | Status: ");
    uart2_write(buff.status);
    uart2_write(" \n");
}
