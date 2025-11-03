#include "core/cortex-m4/uart.h"
#include <stdint.h>
#define CORTEX_M4
#include "navhal.h"

#define FLASH_BASE        0x40023C00UL
#define FLASH_KEYR        (*(volatile uint32_t*)(FLASH_BASE + 0x04))
#define FLASH_SR          (*(volatile uint32_t*)(FLASH_BASE + 0x0C))
#define FLASH_CR          (*(volatile uint32_t*)(FLASH_BASE + 0x10))
#define FLASH_OPTCR       (*(volatile uint32_t*)(FLASH_BASE + 0x14))

/* Flash key values */
#define FLASH_KEY1        0x45670123U
#define FLASH_KEY2        0xCDEF89ABU

/* FLASH_CR bits */
#define FLASH_CR_PG       (1U << 0)
#define FLASH_CR_SER      (1U << 1)
#define FLASH_CR_MER      (1U << 2)
#define FLASH_CR_SNB_Pos  3U
#define FLASH_CR_SNB_Msk  (0xFU << FLASH_CR_SNB_Pos)
#define FLASH_CR_PSIZE_Pos 8U
#define FLASH_CR_PSIZE_Msk (0x3U << FLASH_CR_PSIZE_Pos)
#define FLASH_CR_STRT     (1U << 16)
#define FLASH_CR_LOCK     (1U << 31)

/* FLASH_SR bits */
#define FLASH_SR_BSY      (1U << 16)

/* Flash memory base */
#define FLASH_BASE_ADDR   0x08000000UL
#define SECTOR5_ADDR      0x08020000UL

/* Value to write */
#define DATA_VALUE        12345678U

/* ---------------- Function Prototypes ---------------- */
void flash_unlock(void);
void flash_lock(void);
void flash_wait(void);
void flash_erase_sector(uint8_t sector);
void flash_program_word(uint32_t addr, uint32_t data);
uint32_t flash_read_word(uint32_t addr);

/* ---------------- Function Definitions ---------------- */

void flash_wait(void) {
    while (FLASH_SR & FLASH_SR_BSY);
}

void flash_unlock(void) {
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

void flash_lock(void) {
    FLASH_CR |= FLASH_CR_LOCK;
}

void flash_erase_sector(uint8_t sector) {
    flash_wait();
    FLASH_CR &= ~FLASH_CR_SNB_Msk;
    FLASH_CR |= FLASH_CR_SER | ((sector & 0xF) << FLASH_CR_SNB_Pos);
    FLASH_CR |= FLASH_CR_STRT;
    flash_wait();
    FLASH_CR &= ~FLASH_CR_SER;
}

void flash_program_word(uint32_t addr, uint32_t data) {
    flash_wait();
    FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
    FLASH_CR |= (0x2U << FLASH_CR_PSIZE_Pos);  // x32 programming
    FLASH_CR |= FLASH_CR_PG;

    *(volatile uint32_t*)addr = data;

    flash_wait();
    FLASH_CR &= ~FLASH_CR_PG;
}

uint32_t flash_read_word(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

/* ---------------- Main Program ---------------- */

int main(void) {
    systick_init(1000);   /**< Initialize SysTick with 1 ms tick */
    uart2_init(9600);     /**< Initialize UART2 at 9600 baud */
      // flash_unlock();

      // // Erase Sector 5
      // flash_erase_sector(5);

      // // // Write value
      // flash_program_word(SECTOR5_ADDR, DATA_VALUE);

      // // // Lock flash
      // flash_lock();
      int val = flash_read_word(SECTOR5_ADDR);
      // Read back
      uart2_write(val);
      uart2_write("\n\r");
    // At this point, val should equal 0x12345678
    while (1);
}

