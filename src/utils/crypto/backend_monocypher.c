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
 * @file backend_monocypher.c
 * @brief hal_boot_crypto over Monocypher. The default backend: 10,768 bytes and
 *        29.2 ms per verify on an F401 at 84 MHz, and the only one of the three
 *        that rejects every malleable signature in the Wycheproof corpus.
 */
#include "common/hal_boot_crypto.h"
#include "monocypher-ed25519.h"

hal_status_t hal_boot_ed25519_verify(const uint8_t *sig, const uint8_t *pk,
                                    const uint8_t *msg, size_t len) {
  if (sig == NULL || pk == NULL || (msg == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;
  return crypto_ed25519_check(sig, pk, msg, len) == 0 ? HAL_OK : HAL_ERR;
}

hal_status_t hal_boot_hash(uint8_t *out, const uint8_t *data, size_t len) {
  if (out == NULL || (data == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;
  crypto_sha512(out, data, len);
  return HAL_OK;
}
