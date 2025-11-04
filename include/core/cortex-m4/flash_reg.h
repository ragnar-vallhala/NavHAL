/**
 * @file core/cortex-m4/flash.h
 * @brief Cortex-M4 Flash memory interface definitions.
 *
 * @details
 * This header defines the base address and configuration bit positions
 * for the Flash memory interface registers on Cortex-M4 microcontrollers.
 * These macros are used by the HAL to configure Flash access latency and
 * other Flash-related settings.
 *
 * Macros:
 * - `FLASH_INTERFACE_REGISTER` : Base address of the Flash interface registers.
 * - `FLASH_ACR_LATENCY_BIT`    : Bit position for Flash access latency configuration.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_FLASH_REG_H
#define CORTEX_M4_FLASH_REG_H

#define FLASH_INTERFACE_REGISTER 0x40023C00 /**< Flash Interface base address */
#define FLASH_ACR_LATENCY_BIT 0             /**< Flash ACR Latency bit position */

#define FLASH_BASE 0x40023C00UL
#define FLASH_KEYR (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_SR (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* Flash key values */
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

/* FLASH_CR bits */
#define FLASH_CR_PG (1U << 0)
#define FLASH_CR_SER (1U << 1)
#define FLASH_CR_MER (1U << 2)
#define FLASH_CR_SNB_Pos 3U
#define FLASH_CR_SNB_Msk (0xFU << FLASH_CR_SNB_Pos)
#define FLASH_CR_PSIZE_Pos 8U
#define FLASH_CR_PSIZE_Msk (0x3U << FLASH_CR_PSIZE_Pos)
#define FLASH_CR_STRT (1U << 16)
#define FLASH_CR_LOCK (1U << 31)

/* FLASH_SR bits */
#define FLASH_SR_BSY (1U << 16)

/* Flash memory base */
#define FLASH_BASE_ADDR 0x08000000UL
#define SECTOR5_ADDR 0x08020000UL

#define FLASH_STORAGE_START SECTOR5_ADDR
#define FLASH_STORAGE_END (SECTOR5_ADDR + 0x20000) // adjust sector size
#define FLASH_PAGE_SIZE 0x400                      // 1 KB per page (depends on MCU)

#define FLASH_MAGIC_NUMBER 0x1A
#define FLASH_EMPTY 0xFF
#define FLASH_VALID 0x01
#define FLASH_DELETED 0x00
#define FLASH_DEFAULT_BLOCK 5

#endif // !CORTEX_M4_FLASH_REG_H
