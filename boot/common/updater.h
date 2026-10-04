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
 * @file updater.h
 * @brief The frame format a loader accepts when it will not hand over.
 *
 * Deliberately small. Every byte of stage-1 is permanent, so the protocol has to
 * be the kind of thing that can be read in one sitting and reimplemented from
 * this header alone -- no length-prefixed nesting, no state beyond "which offset
 * comes next", no negotiation.
 *
 * One frame in, one reply out, over whichever transport spoke first:
 *
 *     'N' 'H' cmd len_lo len_hi payload[len] crc32[4]
 *
 * The CRC covers cmd, both length bytes and the payload, little-endian, hardware
 * CRC-32 on this part. The reply is two bytes: a status and a code, so a host
 * learns why rather than only that.
 *
 * Commands:
 *
 *   'I' info     payload none          -> code carries the protocol version
 *   'E' erase    payload none          -> erase the target partition
 *   'W' write    payload off32 + data  -> program data at off32 from the base
 *   'V' verify   payload none          -> run the real verify over the partition
 *   'R' run      payload none          -> reset, so the normal path re-decides
 *
 * Each stage serves exactly one partition -- stage-1 writes stage-2, stage-2
 * writes the app -- which is why the target is a parameter rather than a
 * constant. A loader that could write every partition is a loader whose bugs
 * reach everything, and the one that can rewrite stage-2 is already the one
 * protected by WRP.
 *
 * Erase is explicit rather than implied by the first write, because a host that
 * dies mid-transfer should leave the partition erased and obviously incomplete
 * rather than half-old and half-new. Stage-1 verifies before jumping either way,
 * so an interrupted recovery costs another attempt and never a boot into
 * nonsense.
 */
#ifndef BOOT_UPDATER_H
#define BOOT_UPDATER_H

#include <stdbool.h>
#include <stdint.h>

#define RECOVERY_SYNC0 'N'
#define RECOVERY_SYNC1 'H'

#define RECOVERY_CMD_INFO   'I'
#define RECOVERY_CMD_ERASE  'E'
#define RECOVERY_CMD_WRITE  'W'
#define RECOVERY_CMD_VERIFY 'V'
#define RECOVERY_CMD_RUN    'R'
#define RECOVERY_CMD_FLOOR  'F' /**< payload u32: set the rollback floor */

/** @brief Reply status bytes. */
#define RECOVERY_ACK 'K'
#define RECOVERY_NAK 'N'

/** @brief Reply codes, sent with ::RECOVERY_NAK. */
#define RECOVERY_ERR_NONE      0x00u
#define RECOVERY_ERR_CRC       0x01u /**< frame CRC did not match */
#define RECOVERY_ERR_CMD       0x02u /**< unknown command */
#define RECOVERY_ERR_LEN       0x03u /**< payload length impossible for the cmd */
#define RECOVERY_ERR_RANGE     0x04u /**< write outside the target partition */
#define RECOVERY_ERR_FLASH     0x05u /**< erase or program reported a failure */
#define RECOVERY_ERR_VERIFY    0x06u /**< the image in flash does not verify */
#define RECOVERY_ERR_NOT_ERASED 0x07u /**< write before an erase */
#define RECOVERY_ERR_ROLLBACK  0x08u /**< image version is below the floor */
#define RECOVERY_ERR_STORE     0x09u /**< the KV store refused the floor */

/** @brief Protocol version, returned by ::RECOVERY_CMD_INFO. */
#define RECOVERY_VERSION 1u

/** @brief Largest payload a frame may carry: a 4-byte offset plus 256 data. */
#define RECOVERY_MAX_PAYLOAD 260u


/* The partition a stage manages must not contain the key-value store's sectors:
 * the raw flash API refuses to erase them -- correctly, the store owns them --
 * so an update would fail at the erase with nothing to explain it. That is not a
 * hypothetical: with the store left at its default 6 and 7, stage-2's app
 * partition is exactly those sectors and every erase was refused.
 *
 * Checked here rather than in each stage, because the next stage to be written
 * will make the same assumption.
 */
#if defined(NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR) &&                          \
    (NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR + 0) > 0
_Static_assert(NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR < 4 &&
                   NAVHAL_CONFIG_FLASH_KV_SECONDARY_SECTOR < 4,
               "a bootloader build needs the KV store below the stage-2 "
               "partition: set CONFIG_FLASH_KV_PRIMARY_SECTOR=2 and "
               "SECONDARY=3, or the app partition's sectors cannot be erased");
#else
#error "a bootloader image needs CONFIG_FLASH_KV_PRIMARY_SECTOR / SECONDARY set (2 and 3); the defaults 6 and 7 are inside the app partition"
#endif

/**
 * @brief A transport, as recovery needs it: one byte in, one byte out.
 *
 * Two function pointers rather than a transport enum, so the frame parser has no
 * idea whether it is talking over a UART or a CDC endpoint and there is only one
 * copy of it.
 */
typedef struct {
  /** Blocking read of one byte. Returns false if nothing arrived in time. */
  bool (*get)(uint8_t *out);
  /** Write one byte. */
  void (*put)(uint8_t b);
} recovery_transport_t;

/**
 * @brief The partition a stage is willing to rewrite, and how it judges it.
 */
typedef struct {
  uint32_t base;      /**< Partition base: the image header. */
  uint32_t size;      /**< Partition size in bytes. */
  uint32_t max_body;  /**< Largest body, header already taken off. */
  const uint8_t *sectors;  /**< Sector numbers to erase. */
  uint8_t sector_count;    /**< How many. */
  /** The verifier this stage uses on its own boot path -- shared on purpose, so
      a host is never told an image is good by a rule the boot path disagrees
      with. */
  bool (*verify)(uint32_t base, uint32_t max_body);
  /** Rollback floor, or NULL when the stage does not enforce one. Stage-1 does
      not: stage-2 is not versioned against a floor, since the thing that would
      roll it back is the only thing that can write it. */
  bool (*floor_ok)(uint32_t version);
  /** Set the floor, or NULL. */
  bool (*floor_set)(uint32_t version);
} updater_target_t;

/**
 * @brief Serve frames until a RUN command resets the board.
 *
 * Never returns. Kicks the watchdog, because an erase outlasts any sane timeout
 * and a host that walks away must not leave the board resetting in a loop.
 */
void updater_serve(const recovery_transport_t *t, const updater_target_t *tgt);

#endif /* BOOT_UPDATER_H */
