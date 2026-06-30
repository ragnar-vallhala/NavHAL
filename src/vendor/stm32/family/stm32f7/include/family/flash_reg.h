/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file flash_reg.h
 * @brief STM32F767 Flash memory interface definitions (RM0410 §3).
 *
 * @details
 * The ACR / KEYR / SR / CR register block, base address (0x40023C00), unlock
 * keys and CR/SR bit layout match the STM32F4, so `flash.c` and `clock_f7.c`
 * drive the F7 flash unchanged — only the **sector map** differs and is encoded
 * here.
 *
 * STM32F767ZI is a single 2 MB bank (nDBANK=1, default) of 12 sectors:
 *   - sectors 0–3:  32 KB each
 *   - sector  4:   128 KB
 *   - sectors 5–11: 256 KB each
 * (4·32 + 128 + 7·256 = 2048 KB). The SNB field in FLASH_CR is 5 bits on the F7
 * (vs 4 on the F4) to address these higher sector numbers.
 */

#ifndef CORTEX_M7_FLASH_REG_H
#define CORTEX_M7_FLASH_REG_H

#define FLASH_INTERFACE_REGISTER 0x40023C00 /**< Flash Interface base address */
#define FLASH_ACR_LATENCY_BIT 0             /**< Flash ACR Latency bit position */

#define FLASH_BASE 0x40023C00UL
#define FLASH_KEYR (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_SR (*(volatile uint32_t *)(FLASH_BASE + 0x0C))
#define FLASH_CR (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR (*(volatile uint32_t *)(FLASH_BASE + 0x14))


#ifdef __cplusplus
extern "C" {
#endif
/* Flash key values */
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

/* FLASH_CR bits */
#define FLASH_CR_PG (1U << 0)
#define FLASH_CR_SER (1U << 1)
#define FLASH_CR_MER (1U << 2)
#define FLASH_CR_SNB_Pos 3U
#define FLASH_CR_SNB_Msk (0x1FU << FLASH_CR_SNB_Pos) /**< 5-bit SNB on F7 */
#define FLASH_CR_PSIZE_Pos 8U
#define FLASH_CR_PSIZE_Msk (0x3U << FLASH_CR_PSIZE_Pos)
#define FLASH_CR_STRT (1U << 16)
#define FLASH_CR_LOCK (1U << 31)

/* FLASH_SR bits */
#define FLASH_SR_BSY (1U << 16)

/* Flash memory base + STM32F767 single-bank 2 MB sector addresses */
#define FLASH_BASE_ADDR 0x08000000UL
#define SECTOR0_ADDR  0x08000000UL //  32 KB
#define SECTOR1_ADDR  0x08008000UL //  32 KB
#define SECTOR2_ADDR  0x08010000UL //  32 KB
#define SECTOR3_ADDR  0x08018000UL //  32 KB
#define SECTOR4_ADDR  0x08020000UL // 128 KB
#define SECTOR5_ADDR  0x08040000UL // 256 KB
#define SECTOR6_ADDR  0x08080000UL // 256 KB
#define SECTOR7_ADDR  0x080C0000UL // 256 KB
#define SECTOR8_ADDR  0x08100000UL // 256 KB
#define SECTOR9_ADDR  0x08140000UL // 256 KB
#define SECTOR10_ADDR 0x08180000UL // 256 KB
#define SECTOR11_ADDR 0x081C0000UL // 256 KB

/** @brief Sector size (bytes) for sector @p n: 32 KB (0–3), 128 KB (4), 256 KB (5–11). */
#define FLASH_SECTOR_SIZE(n) \
    ((n) <= 3 ? 0x8000UL : ((n) == 4 ? 0x20000UL : 0x40000UL))

/** @brief Base address of sector @p n (0–11). */
#define FLASH_SECTOR_ADDR(n)                                                   \
    ((n) == 0 ? SECTOR0_ADDR : (n) == 1 ? SECTOR1_ADDR                         \
                           : (n) == 2 ? SECTOR2_ADDR                           \
                           : (n) == 3 ? SECTOR3_ADDR                           \
                           : (n) == 4 ? SECTOR4_ADDR                           \
                           : (n) == 5 ? SECTOR5_ADDR                           \
                           : (n) == 6 ? SECTOR6_ADDR                           \
                           : (n) == 7 ? SECTOR7_ADDR                           \
                           : (n) == 8 ? SECTOR8_ADDR                           \
                           : (n) == 9 ? SECTOR9_ADDR                           \
                           : (n) == 10 ? SECTOR10_ADDR                         \
                                       : SECTOR11_ADDR)

/* Key/value storage uses two 256 KB sectors near the top of flash, well clear
 * of application code at the bottom (sector 0). */
#define PRIMARY_FLASH_SECTOR 6
#define SECONDARY_FLASH_SECTOR 7

#if PRIMARY_FLASH_SECTOR < 0 || PRIMARY_FLASH_SECTOR > 11
#error "[NAVHAL FLASH] PRIMARY_FLASH_SECTOR must be between 0 and 11"
#endif

#if SECONDARY_FLASH_SECTOR < 0 || SECONDARY_FLASH_SECTOR > 11
#error "[NAVHAL FLASH] SECONDARY_FLASH_SECTOR must be between 0 and 11"
#endif

#if PRIMARY_FLASH_SECTOR == SECONDARY_FLASH_SECTOR
#error "[NAVHAL FLASH] PRIMARY_FLASH_SECTOR and SECONDARY_FLASH_SECTOR must be different"
#endif

#define PRIMARY_FLASH_SECTOR_SIZE   FLASH_SECTOR_SIZE(PRIMARY_FLASH_SECTOR)
#define SECONDARY_FLASH_SECTOR_SIZE FLASH_SECTOR_SIZE(SECONDARY_FLASH_SECTOR)

#define FLASH_PRIMARY_STORAGE_START FLASH_SECTOR_ADDR(PRIMARY_FLASH_SECTOR)
#define FLASH_PRIMARY_STORAGE_END                                              \
    (FLASH_PRIMARY_STORAGE_START + PRIMARY_FLASH_SECTOR_SIZE)

#define FLASH_SECONDARY_STORAGE_START FLASH_SECTOR_ADDR(SECONDARY_FLASH_SECTOR)
#define FLASH_SECONDARY_STORAGE_END                                            \
    (FLASH_SECONDARY_STORAGE_START + SECONDARY_FLASH_SECTOR_SIZE)

#define FLASH_MAGIC_NUMBER 0x1A
#define FLASH_EMPTY 0xFF
#define FLASH_VALID 0x01
#define FLASH_DELETED 0x00
#define FLASH_DEFAULT_BLOCK 5


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !CORTEX_M7_FLASH_REG_H
