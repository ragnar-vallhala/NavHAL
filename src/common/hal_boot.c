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
 * @file hal_boot.c
 * @brief Boot block and console watcher.
 *
 * @details
 * Portable by construction: no register access and no transport dependency.
 * The application feeds bytes in from whichever interrupt its console
 * delivers them on, and the only hardware this touches is the reset itself.
 * That is also what makes the matcher testable on the host.
 */

#include "common/hal_boot.h"

#if NAVHAL_CONFIG_BOOT_SNIFFER

#include "common/hal_reset.h"
#include <stddef.h>

/* The boot block lives outside the region the startup code zeroes, which is
 * what `.noinit` means in every one of this project's linker scripts. Placing
 * it anywhere else would leave it zeroed by the time anything read it, and the
 * failure would look like "the request is never honoured" rather than like a
 * linker problem. */
volatile hal_boot_block_t _sboot __attribute__((section(".noinit"), used));

/* Eight distinct bytes, so no proper prefix of the sequence is also a suffix
 * and the matcher's single-byte resync is correct. Chosen outside printable
 * ASCII so ordinary console traffic cannot start a partial match, and with no
 * 0x00 or 0xFF, which are what an idle or shorted line produces. */
const uint8_t hal_boot_seq[HAL_BOOT_SEQ_LEN] = {0xB0, 0x07, 0xC0, 0xDE,
                                                0xA5, 0x3C, 0x69, 0x96};

/* Both are written from interrupt context and read from thread context. */
static volatile uint8_t s_pos;
static volatile bool s_entry_disabled;
static hal_boot_prepare_cb_t s_prepare;

/* magic ^ request ^ attempts ^ a constant. The constant is what stops an
 * all-zero or all-ones block -- the two states uninitialised SRAM is most
 * likely to be found in -- from checksumming as valid. */
static uint32_t block_check(uint32_t magic, uint32_t request,
                            uint32_t attempts) {
  return magic ^ request ^ attempts ^ 0xA5A5A5A5u;
}

static void block_seal(uint32_t request, uint32_t attempts) {
  _sboot.magic = HAL_BOOT_MAGIC;
  _sboot.request = request;
  _sboot.attempts = attempts;
  /* Last, always: a power cut before this line leaves a block that fails
   * validation, which is the safe outcome. A block sealed first and filled in
   * afterwards would read as valid while holding whatever was there before. */
  _sboot.check = block_check(HAL_BOOT_MAGIC, request, attempts);
}

bool hal_boot_block_valid(void) {
  return _sboot.magic == HAL_BOOT_MAGIC &&
         _sboot.check == block_check(_sboot.magic, _sboot.request,
                                     _sboot.attempts);
}

hal_status_t hal_boot_block_init(void) {
  if (!hal_boot_block_valid()) {
    block_seal(HAL_BOOT_REQ_NONE, 0u); /* cold boot, or RAM lost its contents */
  }
  return HAL_OK;
}

uint32_t hal_boot_get_request(void) {
  return hal_boot_block_valid() ? _sboot.request : HAL_BOOT_REQ_NONE;
}

hal_status_t hal_boot_clear_request(void) {
  if (!hal_boot_block_valid()) {
    return HAL_ERR_NOT_INITIALIZED;
  }
  block_seal(HAL_BOOT_REQ_NONE, _sboot.attempts);
  return HAL_OK;
}

uint32_t hal_boot_get_attempts(void) {
  return hal_boot_block_valid() ? _sboot.attempts : 0u;
}

hal_status_t hal_boot_mark_healthy(void) {
  if (!hal_boot_block_valid()) {
    return HAL_ERR_NOT_INITIALIZED;
  }
  block_seal(_sboot.request, 0u);
  return HAL_OK;
}

/* -------------------------------------------------------------------------- *
 * Watcher
 * -------------------------------------------------------------------------- */

void hal_boot_match_reset(void) { s_pos = 0u; }

void hal_boot_match_byte(uint8_t b) {
  uint8_t pos = s_pos;

  if (b == hal_boot_seq[pos]) {
    pos++;
    if (pos == HAL_BOOT_SEQ_LEN) {
      s_pos = 0u; /* before the request: a refused entry must not re-fire */
      (void)hal_boot_request();
      return;
    }
    s_pos = pos;
    return;
  }

  /* Resync. Retrying the current byte against seq[0] is enough only because
   * the sequence has no proper prefix that is also a suffix; see the note in
   * hal_boot.h. Without that property a real failure table is required, and
   * every sequence arriving mid-stream would be missed. */
  s_pos = (b == hal_boot_seq[0]) ? 1u : 0u;
}

void hal_boot_feed(const uint8_t *data, uint16_t len) {
  if (data == NULL) {
    return;
  }
  for (uint16_t i = 0u; i < len; i++) {
    hal_boot_match_byte(data[i]);
  }
}

/* -------------------------------------------------------------------------- *
 * Entry policy
 * -------------------------------------------------------------------------- */

void hal_boot_entry_disable(void) { s_entry_disabled = true; }
void hal_boot_entry_enable(void) { s_entry_disabled = false; }
bool hal_boot_entry_is_disabled(void) { return s_entry_disabled; }

void hal_boot_set_prepare(hal_boot_prepare_cb_t cb) { s_prepare = cb; }

hal_status_t hal_boot_request(void) {
  if (s_entry_disabled) {
    return HAL_ERR_BUSY;
  }

  /* The attempt count carries across: this reset is deliberate, but it is not
   * evidence that the application is healthy. Only hal_boot_mark_healthy is. */
  block_seal(HAL_BOOT_REQ_LOADER, hal_boot_get_attempts());

  if (s_prepare != NULL) {
    s_prepare();
  }

  /* hal_system_reset issues its own barriers around SYSRESETREQ and does not
   * return. Reaching the line below means the reset did not take. */
  (void)hal_system_reset();
  return HAL_ERR;
}

#endif /* NAVHAL_CONFIG_BOOT_SNIFFER */
