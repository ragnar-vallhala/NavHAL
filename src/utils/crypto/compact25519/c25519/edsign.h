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

/* Edwards curve signature system
 * Daniel Beer <dlbeer@gmail.com>, 22 Apr 2014
 *
 * This file is in the public domain.
 */

#ifndef EDSIGN_H_
#define EDSIGN_H_

#ifndef COMPACT_DISABLE_ED25519
#include <stdint.h>
#include <stddef.h>

/* This is the Ed25519 signature system, as described in:
 *
 *     Daniel J. Bernstein, Niels Duif, Tanja Lange, Peter Schwabe, Bo-Yin
 *     Yang. High-speed high-security signatures. Journal of Cryptographic
 *     Engineering 2 (2012), 77-89. Document ID:
 *     a1a62a2f76d23f65d622484ddd09caf8. URL:
 *     http://cr.yp.to/papers.html#ed25519. Date: 2011.09.26.
 *
 * The format and calculation of signatures is compatible with the
 * Ed25519 implementation in SUPERCOP. Note, however, that our secret
 * keys are half the size: we don't store a copy of the public key in
 * the secret key (we generate it on demand).
 */

/* Any string of 32 random bytes is a valid secret key. There is no
 * clamping of bits, because we don't use the key directly as an
 * exponent (the exponent is derived from part of a key expansion).
 */
#define EDSIGN_SECRET_KEY_SIZE  32

/* Given a secret key, produce the public key (a packed Edwards-curve
 * point).
 */
#define EDSIGN_PUBLIC_KEY_SIZE  32

void edsign_sec_to_pub(uint8_t *pub, const uint8_t *secret);

/* Produce a signature for a message. */
#define EDSIGN_SIGNATURE_SIZE  64

void edsign_sign(uint8_t *signature, const uint8_t *pub,
		 const uint8_t *secret,
		 const uint8_t *message, size_t len);

/* Verify a message signature. Returns non-zero if ok. */
uint8_t edsign_verify(const uint8_t *signature, const uint8_t *pub,
		      const uint8_t *message, size_t len);

#endif
#endif
