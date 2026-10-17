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

#include <stddef.h>
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


/* ---- Image header ------------------------------------------------------- *
 *
 * Stage-2 and the app each carry one; stage-1 does not. The signature covers
 * the header's first 12 bytes and the body, so version and length cannot be
 * edited without invalidating it.
 */

/**
 * @brief Image header magic, "NHIM" in a little-endian hex dump.
 *
 * Deliberately not ::HAL_BOOT_MAGIC. That one marks the boot block in .noinit
 * RAM, and its "uninitialised SRAM could hold this by chance" reasoning is about
 * a different structure in a different memory. Sharing one value would make a
 * stray RAM pattern look like a valid image header and vice versa.
 */
#define HAL_BOOTMAP_IMAGE_MAGIC 0x4D49484EUL

/**
 * @brief The signed image header, as it sits in flash.
 *
 * @c digest covers @c magic, @c version and @c length followed by the body --
 * the first 12 bytes of this structure, not all of it, because the signature
 * and the padding cannot be inside what they protect.
 */
typedef struct {
  uint32_t magic;    /**< ::HAL_BOOTMAP_IMAGE_MAGIC. */
  uint32_t version;  /**< Monotonic; checked against the KV rollback floor. */
  uint32_t length;   /**< Body bytes after the header. Clamped to the
                          partition maximum BEFORE it bounds the hash: an
                          unvalidated length is how a verifier gets walked off
                          the end of flash into bytes an attacker chose. */
  uint8_t digest[32]; /**< SHA-256 over the 12 bytes above, then the body. */
  uint8_t sig[64];    /**< Ed25519 over @c digest. */
  uint8_t pad[HAL_BOOTMAP_HEADER_SIZE - 12u - 32u - 64u];
} hal_boot_image_header_t;

/** @brief Bytes of this header the digest covers, before the body. */
#define HAL_BOOTMAP_SIGNED_PREFIX 12u

_Static_assert(sizeof(hal_boot_image_header_t) == HAL_BOOTMAP_HEADER_SIZE,
               "the image header must fill the space before the payload");
_Static_assert(offsetof(hal_boot_image_header_t, digest) == 0x0C,
               "digest must sit at +0x0C, where the format says");
_Static_assert(offsetof(hal_boot_image_header_t, sig) == 0x2C,
               "signature must sit at +0x2C, where the format says");

#endif /* NAVHAL_CONFIG_FAMILY_STM32F4 */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_BOOTMAP_H */
