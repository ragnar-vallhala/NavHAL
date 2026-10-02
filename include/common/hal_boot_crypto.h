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
 * @file hal_boot_crypto.h
 * @brief What the bootloader needs of a signature: verify one, and hash an
 *        image. Nothing else, and nothing that only a signing host does.
 *
 * Three backends implement this, picked with `CONFIG_BOOT_ED25519_*`. They are
 * not interchangeable on every axis -- see the Kconfig help for the measured
 * flash, speed and malleability differences -- but they agree on this contract:
 * a signature made by the signing tool verifies, and one that was tampered with
 * does not.
 *
 * Deliberately absent: key generation and signing. Stage-1 verifies and nothing
 * more, so a backend's signing half is never linked into it.
 */
#ifndef HAL_BOOT_CRYPTO_H
#define HAL_BOOT_CRYPTO_H

#include "common/hal_status.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Ed25519 public key, signature and image digest sizes, in bytes. */
#define HAL_BOOT_PUBKEY_SIZE 32u
#define HAL_BOOT_SIG_SIZE    64u
#define HAL_BOOT_DIGEST_SIZE 64u /**< SHA-512; the image digest reuses the hash
                                      Ed25519 already carries. */

/**
 * @brief Verify an Ed25519 signature over @p msg.
 *
 * @param sig  64-byte signature.
 * @param pk   32-byte public key.
 * @param msg  Message, usually a digest rather than the image itself.
 * @param len  Length of @p msg in bytes; 0 is valid.
 * @return ::HAL_OK when the signature is good, ::HAL_ERR when it does not
 *         verify, ::HAL_ERR_INVALID_ARG for a NULL pointer or a length this
 *         backend cannot take.
 *
 * A rejection and a bad argument are different answers and have different
 * codes: a caller that cannot tell them apart would treat a NULL pointer as a
 * forged image, or worse, the reverse. Only ::HAL_OK means boot it.
 */
hal_status_t hal_boot_ed25519_verify(const uint8_t *sig, const uint8_t *pk,
                                    const uint8_t *msg, size_t len);

/**
 * @brief Hash @p len bytes of @p data into @p out.
 *
 * SHA-512, because Ed25519 contains it by construction: reusing it costs 58
 * bytes of flash where a separate SHA-256 costs about 1.3 KB. It is the slower
 * hash per byte, which is the trade -- see the roadmap's measurements.
 *
 * @param out  ::HAL_BOOT_DIGEST_SIZE bytes.
 */
hal_status_t hal_boot_hash(uint8_t *out, const uint8_t *data, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_BOOT_CRYPTO_H */
