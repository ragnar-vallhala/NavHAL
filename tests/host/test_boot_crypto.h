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

/** @file test_boot_crypto.h @brief Suite: every Ed25519 backend vs Wycheproof. */
#ifndef TEST_BOOT_CRYPTO_H
#define TEST_BOOT_CRYPTO_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_boot_crypto_corpus_was_found(void);
void test_boot_crypto_every_backend_accepts_every_valid_signature(void);
void test_boot_crypto_every_backend_rejects_every_invalid_signature(void);
void test_boot_crypto_malleability_is_what_kconfig_claims(void);
void test_boot_crypto_digest_matches_fips_180_4(void);
void test_boot_crypto_digest_handles_a_long_message(void);
void test_boot_crypto_split_hash_equals_whole(void);
void test_boot_crypto_hash_rejects_a_null_out(void);

extern const navtest_suite_t test_boot_crypto_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TEST_BOOT_CRYPTO_H */
