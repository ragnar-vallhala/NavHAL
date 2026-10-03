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
 * @file hal_bootmap.h
 * @brief Where each image lives in flash, in one place.
 *
 * The bootloader, the signing tool and the linker scripts all have to agree on
 * these numbers, and the cost of them disagreeing is a board that erases its own
 * code. They are stated once here and derived everywhere else.
 *
 * STM32F401RE, 512 KiB in eight sectors:
 *
 *     sector 0-1   0x08000000  32K   stage-1, write-protected, never erased
 *     sector 2     0x08008000  16K   KV primary
 *     sector 3     0x0800C000  16K   KV secondary
 *     sector 4     0x08010000  64K   stage-2
 *     sector 5-7   0x08020000  384K  application
 *
 * Stage-1 carries no header. It is the root of trust, protected by WRP rather
 * than by a signature, and the CPU boots straight into its vector table.
 *
 * The F767 has a different sector map and needs its own table before that port
 * adopts any of this, which is why every constant here is guarded on the family.
 */
#ifndef HAL_BOOTMAP_H
#define HAL_BOOTMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NAVHAL_CONFIG_FAMILY_STM32F4)

/** @brief Stage-1: sectors 0-1, no header, WRP'd. */
#define HAL_BOOTMAP_STAGE1_BASE 0x08000000UL
#define HAL_BOOTMAP_STAGE1_SIZE 0x00008000UL /* 32 KiB */

/** @brief The KV store's two sectors, erased independently for compaction. */
#define HAL_BOOTMAP_KV_BASE          0x08008000UL
#define HAL_BOOTMAP_KV_SIZE          0x00008000UL /* 32 KiB, two 16K sectors */
#define HAL_BOOTMAP_KV_PRIMARY_SEC   2
#define HAL_BOOTMAP_KV_SECONDARY_SEC 3

/** @brief Stage-2: sector 4, one erase unit. */
#define HAL_BOOTMAP_STAGE2_BASE 0x08010000UL
#define HAL_BOOTMAP_STAGE2_SIZE 0x00010000UL /* 64 KiB */

/** @brief Application: sectors 5-7. */
#define HAL_BOOTMAP_APP_BASE 0x08020000UL
#define HAL_BOOTMAP_APP_SIZE 0x00060000UL /* 384 KiB */

/**
 * @brief Bytes of image header before the payload, for a signed partition.
 *
 * Stage-2 and the app carry one; stage-1 does not. 512 keeps the payload's
 * vector table at a 512-byte boundary, which the Cortex-M VTOR requires.
 */
#define HAL_BOOTMAP_HEADER_SIZE 512UL

#define HAL_BOOTMAP_STAGE2_PAYLOAD (HAL_BOOTMAP_STAGE2_BASE + HAL_BOOTMAP_HEADER_SIZE)
#define HAL_BOOTMAP_APP_PAYLOAD    (HAL_BOOTMAP_APP_BASE + HAL_BOOTMAP_HEADER_SIZE)

/** @brief Usable payload bytes, header already taken off. */
#define HAL_BOOTMAP_STAGE2_USABLE (HAL_BOOTMAP_STAGE2_SIZE - HAL_BOOTMAP_HEADER_SIZE)
#define HAL_BOOTMAP_APP_USABLE    (HAL_BOOTMAP_APP_SIZE - HAL_BOOTMAP_HEADER_SIZE)

/* The partitions must tile the part exactly, with no gap and no overlap. Stated
 * as assertions because the alternative to checking is a board that erases its
 * own code: the KV store sat at 0x08040000 while the linker script claimed the
 * whole 512 KiB, so an app over 256 KiB collided with it silently. */
_Static_assert(HAL_BOOTMAP_STAGE1_BASE + HAL_BOOTMAP_STAGE1_SIZE ==
                   HAL_BOOTMAP_KV_BASE,
               "stage-1 and the KV store must be adjacent");
_Static_assert(HAL_BOOTMAP_KV_BASE + HAL_BOOTMAP_KV_SIZE ==
                   HAL_BOOTMAP_STAGE2_BASE,
               "the KV store and stage-2 must be adjacent");
_Static_assert(HAL_BOOTMAP_STAGE2_BASE + HAL_BOOTMAP_STAGE2_SIZE ==
                   HAL_BOOTMAP_APP_BASE,
               "stage-2 and the app must be adjacent");
_Static_assert(HAL_BOOTMAP_APP_BASE + HAL_BOOTMAP_APP_SIZE == 0x08080000UL,
               "the app must end at the top of the 512 KiB part");
_Static_assert(HAL_BOOTMAP_HEADER_SIZE % 512UL == 0UL,
               "a payload vector table needs 512-byte alignment for VTOR");

#endif /* NAVHAL_CONFIG_FAMILY_STM32F4 */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_BOOTMAP_H */
