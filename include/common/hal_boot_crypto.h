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
#define HAL_BOOT_DIGEST_SIZE 32u /**< SHA-256. */

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
 * SHA-256, and the same one whichever backend is selected -- the signature is
 * made over this digest, so a signed image has to verify after a reconfigure.
 * It is not Ed25519's internal SHA-512, which stays where it is.
 *
 * Chosen over reusing that SHA-512: the app hash is the dominant term in boot
 * time, 676 ms for a 384 KiB image at 144.4 cycles/byte, and SHA-256 is the
 * faster hash. The cost is about 1.3 KB of flash against 58 bytes, which
 * stage-1 can afford -- see the roadmap's measured budget.
 *
 * @param out  ::HAL_BOOT_DIGEST_SIZE bytes of digest.
 * @param data Bytes to hash; may be NULL only when @p len is 0.
 * @param len  Length of @p data in bytes.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL destination or a NULL
 *         source with a non-zero length.
 */
hal_status_t hal_boot_hash(uint8_t *out, const uint8_t *data, size_t len);

/**
 * @brief Hash two ranges as if they were one.
 *
 * What an image digest needs, and the reason this exists rather than a caller
 * concatenating: the signature covers the header's first 12 bytes and then the
 * body, and those are not next to each other -- the header's padding sits
 * between them. The body can be 384 KiB, so copying the two together to hash
 * them is not an option on a part with 96 KiB of RAM.
 *
 * Equivalent to hashing @p a followed by @p b. Either length may be zero.
 *
 * @param out   ::HAL_BOOT_DIGEST_SIZE bytes of digest.
 * @param a     First range -- the image header's signed prefix. May be NULL
 *              only when @p a_len is 0.
 * @param a_len Length of @p a in bytes.
 * @param b     Second range -- the body. May be NULL only when @p b_len is 0.
 * @param b_len Length of @p b in bytes.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL destination or a NULL
 *         range with a non-zero length.
 */
hal_status_t hal_boot_hash_split(uint8_t *out, const uint8_t *a, size_t a_len,
                                const uint8_t *b, size_t b_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_BOOT_CRYPTO_H */
