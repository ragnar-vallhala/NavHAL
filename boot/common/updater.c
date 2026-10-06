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
 * @file updater.c
 * @brief The frame server. See updater.h for the format and why it is this dull.
 *
 * One copy, two stages. Stage-1 points it at stage-2, stage-2 points it at the
 * app, and neither has its own version of a protocol that a host has to match.
 */
#include "updater.h"

#include "common/hal_boot.h"
#include "common/hal_bootmap.h"
#include "common/hal_flash.h"
#include "common/hal_watchdog.h"

#include <string.h>

/* Set by a successful erase, cleared by a reset. A write before an erase is
 * refused rather than attempted: programming into un-erased flash yields the AND
 * of old and new, which verifies as corrupt and looks like a transfer fault. */
static bool erased;

/* CRC-32/MPEG-2 in software, deliberately not hal_crc.
 *
 * The hardware unit on this part takes 32-bit words, and the driver zero-pads a
 * tail that is not a multiple of four -- so two accumulate calls are not the same
 * as one call over the concatenation, because each pads its own tail. A frame CRC
 * over a header and a payload is exactly that case, and a host would have to
 * reproduce the padding to agree. Twenty bytes of table-free loop removes the
 * whole question from the one image that can never be fixed in the field, and
 * drops a peripheral dependency with it.
 *
 * Same parameters the hardware uses: poly 0x04C11DB7, init 0xFFFFFFFF, no
 * reflection, no final XOR.
 */
static uint32_t crc32_mpeg2(uint32_t crc, const uint8_t *data, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    crc ^= (uint32_t)data[i] << 24;
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80000000u) ? ((crc << 1) ^ 0x04C11DB7u) : (crc << 1);
    }
  }
  return crc;
}

static bool read_frame(const recovery_transport_t *t, uint8_t *cmd,
                       uint8_t *payload, uint16_t *len) {
  uint8_t b;

  /* Resync on the two sync bytes rather than trusting alignment: a host that
   * died mid-frame leaves the stream offset, and the next frame has to be
   * findable without a reset. */
  for (;;) {
    (void)hal_watchdog_kick();
    if (!t->get(&b))
      return false;
    if (b != RECOVERY_SYNC0)
      continue;
    if (!t->get(&b))
      return false;
    if (b == RECOVERY_SYNC1)
      break;
  }

  if (!t->get(cmd))
    return false;

  uint8_t lo, hi;
  if (!t->get(&lo) || !t->get(&hi))
    return false;
  uint16_t n = (uint16_t)((uint16_t)lo | ((uint16_t)hi << 8));
  if (n > RECOVERY_MAX_PAYLOAD)
    return false;

  for (uint16_t i = 0; i < n; i++) {
    if (!t->get(&payload[i]))
      return false;
  }

  uint8_t crc_bytes[4];
  for (int i = 0; i < 4; i++) {
    if (!t->get(&crc_bytes[i]))
      return false;
  }

  /* The CRC covers cmd, the two length bytes and the payload, in that order --
   * the bytes as they arrived, so a host computes it over what it sent. */
  uint8_t head[3] = {*cmd, lo, hi};
  uint32_t calc = crc32_mpeg2(0xFFFFFFFFu, head, sizeof head);
  calc = crc32_mpeg2(calc, payload, n);

  uint32_t want = (uint32_t)crc_bytes[0] | ((uint32_t)crc_bytes[1] << 8) |
                  ((uint32_t)crc_bytes[2] << 16) | ((uint32_t)crc_bytes[3] << 24);
  *len = n;
  return calc == want;
}

static void reply(const recovery_transport_t *t, uint8_t status, uint8_t code) {
  t->put(status);
  t->put(code);
}

static uint8_t do_write(const updater_target_t *tgt, const uint8_t *payload,
                        uint16_t len) {
  if (len < 5u || len > RECOVERY_MAX_PAYLOAD)
    return RECOVERY_ERR_LEN;
  if (!erased)
    return RECOVERY_ERR_NOT_ERASED;

  uint32_t off = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                 ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
  uint16_t n = (uint16_t)(len - 4u);

  /* Both ends checked, and against the partition rather than against flash: the
   * raw API refuses stage-1 and the KV sectors anyway, but a loader should know
   * its own bounds rather than rely on being stopped. */
  if (off > tgt->size || n > tgt->size - off)
    return RECOVERY_ERR_RANGE;

  if (hal_flash_raw_program(tgt->base + off, &payload[4], n) != HAL_OK)
    return RECOVERY_ERR_FLASH;
  return RECOVERY_ERR_NONE;
}

void updater_serve(const recovery_transport_t *t,
                   const updater_target_t *tgt) {
  static uint8_t payload[RECOVERY_MAX_PAYLOAD];
  uint8_t cmd = 0;
  uint16_t len = 0;

  erased = false;

  for (;;) {
    (void)hal_watchdog_kick();

    if (!read_frame(t, &cmd, payload, &len)) {
      /* A bad CRC or a truncated frame: say so and resync. Not fatal -- a noisy
       * line should cost a retry, not a recovery session. */
      reply(t, RECOVERY_NAK, RECOVERY_ERR_CRC);
      continue;
    }

    switch (cmd) {
    case RECOVERY_CMD_INFO:
      reply(t, RECOVERY_ACK, (uint8_t)RECOVERY_VERSION);
      break;

    case RECOVERY_CMD_ERASE: {
      /* Every sector of the partition, because a 384 KiB app spans three of them
       * and a half-erased partition is the one state that looks like a transfer
       * fault rather than an incomplete erase. */
      uint8_t err = RECOVERY_ERR_NONE;
      for (uint8_t i = 0; i < tgt->sector_count; i++) {
        (void)hal_watchdog_kick(); /* a 128 KiB erase outlasts any sane timeout */
        if (hal_flash_raw_erase_sector(tgt->sectors[i]) != HAL_OK) {
          err = RECOVERY_ERR_FLASH;
          break;
        }
      }
      erased = (err == RECOVERY_ERR_NONE);
      reply(t, erased ? RECOVERY_ACK : RECOVERY_NAK, err);
      break;
    }

    case RECOVERY_CMD_WRITE: {
      uint8_t err = do_write(tgt, payload, len);
      reply(t, err == RECOVERY_ERR_NONE ? RECOVERY_ACK : RECOVERY_NAK, err);
      break;
    }

    case RECOVERY_CMD_VERIFY:
      /* The real verify, the same function the boot path uses -- a recovery that
       * reported success by a different rule than the one that decides whether
       * the board boots would be worse than no report at all. */
      if (!tgt->verify(tgt->base, tgt->max_body)) {
        reply(t, RECOVERY_NAK, RECOVERY_ERR_VERIFY);
        break;
      }
      /* A signed image that is older than the floor is refused here as well as
       * on the boot path, so a host learns before it resets the board. */
      if (tgt->floor_ok != NULL) {
        const hal_boot_image_header_t *h =
            (const hal_boot_image_header_t *)tgt->base;
        if (!tgt->floor_ok(h->version)) {
          reply(t, RECOVERY_NAK, RECOVERY_ERR_ROLLBACK);
          break;
        }
      }
      reply(t, RECOVERY_ACK, RECOVERY_ERR_NONE);
      break;

    case RECOVERY_CMD_FLOOR: {
      if (tgt->floor_set == NULL) {
        reply(t, RECOVERY_NAK, RECOVERY_ERR_CMD);
        break;
      }
      if (len != 4u) {
        reply(t, RECOVERY_NAK, RECOVERY_ERR_LEN);
        break;
      }
      uint32_t v = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                   ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
      if (tgt->floor_set(v)) {
        reply(t, RECOVERY_ACK, RECOVERY_ERR_NONE);
      } else {
        reply(t, RECOVERY_NAK, RECOVERY_ERR_STORE);
      }
      break;
    }

    case RECOVERY_CMD_RUN:
      /* Refuse to reset into something that will not boot: a reset that lands
       * straight back here tells the host nothing it did not already know. */
      if (!tgt->verify(tgt->base, tgt->max_body)) {
        reply(t, RECOVERY_NAK, RECOVERY_ERR_VERIFY);
        break;
      }

      /* Clear the strike count. The image that earned those strikes has just been
       * replaced, so carrying them forward would send the next boot straight back
       * into recovery -- a board that crashlooped, was repaired, and then refused
       * to run the repair. Found exactly that way on hardware.
       *
       * Safe because it is gated on a verified image: this is not "trust the
       * host", it is "the partition now holds something signed". */
      (void)hal_boot_mark_healthy();

      reply(t, RECOVERY_ACK, RECOVERY_ERR_NONE);
      /* Reset rather than jump: the normal boot path re-reads the request, the
       * attempt count and the signature, so there is one decision point and not
       * two. */
      (void)hal_system_reset();
      break;

    default:
      reply(t, RECOVERY_NAK, RECOVERY_ERR_CMD);
      break;
    }
  }
}
