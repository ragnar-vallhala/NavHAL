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
 * @file hal_boot.h
 * @brief Boot block and console watcher — the way back into the bootloader.
 *
 * @details
 * Two halves of one mechanism. The **boot block** is a small structure in
 * uninitialised RAM that survives a reset, through which the application asks
 * the next boot for something. The **watcher** feeds a byte stream through a
 * matcher and, on seeing the magic sequence, writes that request and resets.
 *
 * The watcher's reason to exist is recovering a board whose application is
 * misbehaving, so it has to keep working when the main loop is wedged. Every
 * byte therefore reaches @c hal_boot_match_byte from an interrupt or from a
 * DMA ring walked by one — never from a drain loop, which stops running at
 * exactly the moment the watcher is needed. See @ref roadmap_bootloader for
 * the transport wiring.
 *
 * Nothing here verifies anything. Entering the bootloader is not a privilege:
 * the bootloader's signature check is what makes an update safe, and this only
 * decides *when* to reboot into it. What it is, is an availability concern —
 * anyone who can write to the console can reboot the board — which is what
 * @c hal_boot_entry_disable exists for.
 *
 * @defgroup HAL_BOOT Boot
 * @ingroup HAL_CORE
 * @{
 */
#ifndef HAL_BOOT_H
#define HAL_BOOT_H

#include "common/hal_config.h"
#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if NAVHAL_CONFIG_BOOT_SNIFFER

/** @brief Marks the boot block as seeded by firmware rather than left by chance. */
#define HAL_BOOT_MAGIC 0xB007C0DEu

/** @brief No request pending; boot normally. */
#define HAL_BOOT_REQ_NONE 0x00000000u

/** @brief Stay in the bootloader instead of starting the application. */
#define HAL_BOOT_REQ_LOADER 0x10ADED00u

/** @brief Consecutive failed boots before the bootloader stops trying. */
#define HAL_BOOT_MAX_ATTEMPTS 3u

/** @brief Length of the sequence that triggers a request. */
#define HAL_BOOT_SEQ_LEN 8u

/**
 * @brief The boot block: what one boot leaves for the next.
 *
 * Lives in `.noinit`, which the startup code neither loads nor zeroes, so it
 * survives a warm reset and is garbage after a cold one. @c check is what
 * tells the two apart.
 */
typedef struct {
  uint32_t magic;    /**< ::HAL_BOOT_MAGIC once seeded. */
  uint32_t request;  /**< ::HAL_BOOT_REQ_NONE or ::HAL_BOOT_REQ_LOADER. */
  uint32_t attempts; /**< Consecutive boots that have not proven liveness. */
  uint32_t check;    /**< Guards the three above; see ::hal_boot_block_valid. */
} hal_boot_block_t;

/** @brief The boot block itself, shared with the bootloader at a fixed address. */
extern volatile hal_boot_block_t _sboot;

/**
 * @brief The byte sequence the watcher looks for.
 *
 * Its bytes are all distinct, and that is a correctness requirement rather
 * than a style choice — see ::hal_boot_match_byte.
 */
extern const uint8_t hal_boot_seq[HAL_BOOT_SEQ_LEN];

/* -------------------------------------------------------------------------- *
 * Boot block
 * -------------------------------------------------------------------------- */

/**
 * @brief Whether the boot block holds something firmware wrote.
 *
 * Uninitialised SRAM could hold ::HAL_BOOT_MAGIC by chance; it will not hold
 * it together with a matching @c check, which is what makes a cold boot safe
 * to detect.
 *
 * @return @c true if the block is self-consistent.
 */
bool hal_boot_block_valid(void);

/**
 * @brief Validate the boot block, seeding it if this was a cold boot.
 *
 * Call once, early, before anything reads a request. Leaves a valid block
 * untouched so a request written before the reset survives to be read.
 *
 * @return ::HAL_OK.
 */
hal_status_t hal_boot_block_init(void);

/**
 * @brief The request left by the previous boot.
 * @return ::HAL_BOOT_REQ_LOADER, or ::HAL_BOOT_REQ_NONE if none or invalid.
 */
uint32_t hal_boot_get_request(void);

/**
 * @brief Clear any pending request, leaving the attempt count alone.
 * @return ::HAL_OK, or ::HAL_ERR_NOT_INITIALIZED if the block is not valid.
 */
hal_status_t hal_boot_clear_request(void);

/**
 * @brief Consecutive boots that have not yet proven themselves.
 * @return The attempt count; 0 if the block is not valid.
 */
uint32_t hal_boot_get_attempts(void);

/**
 * @brief Declare this boot healthy, clearing the attempt count.
 *
 * Call after a liveness threshold the application defines, never at startup:
 * a crash that happens after @c main() would otherwise reset its own strike
 * count forever and never reach ::HAL_BOOT_MAX_ATTEMPTS.
 *
 * @return ::HAL_OK, or ::HAL_ERR_NOT_INITIALIZED if the block is not valid.
 */
hal_status_t hal_boot_mark_healthy(void);

/* -------------------------------------------------------------------------- *
 * Watcher
 * -------------------------------------------------------------------------- */

/**
 * @brief Feed one received byte to the matcher.
 *
 * Safe to call from interrupt context, and designed to be called from there.
 * Fragmentation is irrelevant: a DMA chunk boundary, a 64-byte CDC packet and
 * a single interrupt byte all behave the same, and a sequence split across
 * two transfers still matches.
 *
 * On the final byte of ::hal_boot_seq this calls ::hal_boot_request, which
 * does not return unless entry is disabled or the reset itself fails.
 *
 * @param b The received byte.
 *
 * @note The matcher resyncs by retrying the current byte against the first
 *       byte of the sequence, which is correct only because no proper prefix
 *       of ::hal_boot_seq is also a suffix of it. All-distinct bytes give that
 *       property; a sequence without it needs a real failure table, and would
 *       silently miss any sequence arriving mid-stream.
 */
void hal_boot_match_byte(uint8_t b);

/**
 * @brief Feed a buffer to the matcher.
 *
 * Deliberately shaped as @c hal_usb_cdc_rx_callback_t so CDC can drive it.
 * Note that registering it there takes the stream over rather than tapping
 * it — @c hal_usb_cdc_set_rx_callback delivers bytes to the callback instead
 * of queueing them — so an application that also reads CDC must forward from
 * its own callback rather than registering this one directly.
 *
 * @param data Received bytes; ignored when NULL.
 * @param len  Number of bytes.
 */
void hal_boot_feed(const uint8_t *data, uint16_t len);

/** @brief Drop any partial match, as after a link drop. */
void hal_boot_match_reset(void);

/* -------------------------------------------------------------------------- *
 * Entry policy
 * -------------------------------------------------------------------------- */

/**
 * @brief Refuse to act on a matched sequence.
 *
 * What this disables is the *entry path*, not booting. Matching continues, so
 * nothing is lost from a stream that arrives while entry is refused; the match
 * simply does not reboot the board.
 *
 * A vehicle calls this when the airframe arms: rebooting into the bootloader
 * mid-flight is a fall out of the sky, and anyone able to write to the console
 * can otherwise ask for one.
 */
void hal_boot_entry_disable(void);

/** @brief Allow a matched sequence to enter the bootloader again. */
void hal_boot_entry_enable(void);

/**
 * @brief Whether entry is currently refused.
 * @return @c true if ::hal_boot_entry_disable is in effect. Default @c false,
 *         so a board that never calls either stays recoverable.
 */
bool hal_boot_entry_is_disabled(void);

/** @brief Called before the reset, to be installed with ::hal_boot_set_prepare. */
typedef void (*hal_boot_prepare_cb_t)(void);

/**
 * @brief Run @p cb immediately before the reset.
 *
 * Somewhere to cut throttle or flush a log. It runs in whatever context the
 * matching byte arrived in, which is normally an interrupt, so it must be
 * short and must return. It is not a veto and it is not a rescue: if the
 * application is wedged badly enough not to get here, the reset happens
 * anyway, which is the case this whole mechanism exists for.
 *
 * @param cb Callback, or NULL to remove.
 */
void hal_boot_set_prepare(hal_boot_prepare_cb_t cb);

/**
 * @brief Ask the next boot for the bootloader, and reset into it.
 *
 * Writes the request to the boot block with @c check last, so a power cut
 * part-way through cannot leave a block that reads as valid. Then runs the
 * prepare callback and resets.
 *
 * @retval HAL_ERR_BUSY Entry is disabled; nothing was written.
 * @return Does not return on success. ::HAL_ERR if the reset did not happen.
 */
hal_status_t hal_boot_request(void);

#endif /* NAVHAL_CONFIG_BOOT_SNIFFER */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_BOOT */
#endif /* HAL_BOOT_H */
