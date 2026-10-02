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
 * @file backend_compact25519.c
 * @brief hal_boot_crypto over compact25519 (c25519). The smallest of the three
 *        at 4,918 bytes and the slowest at 55x Monocypher's time; accepts 4 of
 *        the 8 malleable signatures in the Wycheproof corpus.
 *
 * c25519 exposes SHA-512 as a block API rather than a one-shot, so the hash
 * below drives it: whole blocks through sha512_block, then the tail and the
 * length through sha512_final.
 */
#include "common/hal_boot_crypto.h"
#include "compact_ed25519.h"
#include "c25519/sha512.h"
#include <string.h>

hal_status_t hal_boot_ed25519_verify(const uint8_t *sig, const uint8_t *pk,
                                    const uint8_t *msg, size_t len) {
  if (sig == NULL || pk == NULL || (msg == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;
  return compact_ed25519_verify(sig, pk, msg, len) ? HAL_OK : HAL_ERR;
}

hal_status_t hal_boot_hash(uint8_t *out, const uint8_t *data, size_t len) {
  if (out == NULL || (data == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;

  struct sha512_state s;
  sha512_init(&s);

  size_t off = 0;
  while (len - off >= SHA512_BLOCK_SIZE) {
    sha512_block(&s, data + off);
    off += SHA512_BLOCK_SIZE;
  }
  sha512_final(&s, data + off, len);
  sha512_get(&s, out, 0, HAL_BOOT_DIGEST_SIZE);
  return HAL_OK;
}
