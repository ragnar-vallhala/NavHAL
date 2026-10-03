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
 * @file boot_hash.c
 * @brief The image digest: SHA-256, one implementation for every backend.
 *
 * Not part of Ed25519 -- that carries its own SHA-512 internally and always
 * will. This is the hash the signing tool runs over the image and the signature
 * is made over, so it has to be the same function regardless of which backend
 * verifies, or a signed image would stop verifying on a reconfigure.
 *
 * SHA-256 rather than reusing that SHA-512, which would have cost 58 bytes
 * instead of about 1.3 KB: the app hash is the dominant term in boot time at
 * 676 ms for 384 KiB with SHA-512, and stage-1 has the flash to spare.
 */
#include "common/hal_boot_crypto.h"
#include "sha256.h"

hal_status_t hal_boot_hash(uint8_t *out, const uint8_t *data, size_t len) {
  if (out == NULL || (data == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;

  SHA256_CTX ctx;
  sha256_init(&ctx);
  /* The vendored update takes a size_t length, so an image is hashed in one
   * call; it is read straight out of flash and never copied. */
  sha256_update(&ctx, data, len);
  sha256_final(&ctx, out);
  return HAL_OK;
}

hal_status_t hal_boot_hash_split(uint8_t *out, const uint8_t *a, size_t a_len,
                                const uint8_t *b, size_t b_len) {
  if (out == NULL || (a == NULL && a_len != 0u) || (b == NULL && b_len != 0u))
    return HAL_ERR_INVALID_ARG;

  SHA256_CTX ctx;
  sha256_init(&ctx);
  if (a_len != 0u)
    sha256_update(&ctx, a, a_len);
  if (b_len != 0u)
    sha256_update(&ctx, b, b_len);
  sha256_final(&ctx, out);
  return HAL_OK;
}
