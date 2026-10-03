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
 * @file test_boot_crypto.c
 * @brief Every Ed25519 backend against Google Project Wycheproof's EdDSA
 *        corpus, before anything on target trusts one.
 *
 * All three are compiled here at once, called through their own APIs rather than
 * the hal_boot_crypto seam -- the seam defines one set of symbols, so only one
 * backend can wear it per build, and what needs validating is each of the three.
 *
 * Two kinds of assertion. The corpus's "valid" and "invalid" groups are a
 * contract every backend must keep: accept all 84, reject all 41. The
 * malleability group is where they genuinely differ, so the counts are pinned
 * rather than required to agree -- TweetNaCl and compact25519 do not range-check
 * S and accept 4 of 8, Monocypher rejects all 8 (RFC 8032 section 5.2.7). A
 * backend upgrade that changes any of those numbers should have to say so here.
 */
#include "test_boot_crypto.h"
#include "navtest/navtest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c25519/sha512.h"
#include "common/hal_boot_crypto.h"
#include "compact_ed25519.h"
#include "monocypher-ed25519.h"
#include "tweetnacl.h"

#ifndef BOOT_VECTORS_PATH
#define BOOT_VECTORS_PATH "vectors/ed25519_check"
#endif

/* TweetNaCl's signing half references this even though nothing here calls it.
 * Zeros are safe on a host that never generates a key; the on-target backend
 * traps instead, because there a predictable key would be a real one. */
void randombytes(unsigned char *p, unsigned long long n) {
  while (n-- != 0u)
    *p++ = 0u;
}

enum { BK_MONO, BK_TWEET, BK_COMPACT, BK_COUNT };

struct counts {
  int valid_accepted, valid_total;
  int invalid_accepted, invalid_total;
  int malleable_accepted, malleable_total;
};
static struct counts c[BK_COUNT];
static int vectors_read;

static int unhex(const char *h, unsigned char *out, size_t cap, size_t *len) {
  size_t n = strlen(h);
  if (n % 2 || n / 2 > cap)
    return -1;
  *len = n / 2;
  for (size_t i = 0; i < *len; i++) {
    unsigned v;
    if (sscanf(h + 2 * i, "%2x", &v) != 1)
      return -1;
    out[i] = (unsigned char)v;
  }
  return 0;
}

static int accepts(int backend, const unsigned char *sig, const unsigned char *pk,
                   const unsigned char *msg, size_t len) {
  switch (backend) {
  case BK_MONO:
    return crypto_ed25519_check(sig, pk, msg, len) == 0;
  case BK_TWEET: {
    /* crypto_sign_open wants signature and message in one buffer and writes the
     * message back out, so both buffers have to hold the longest vector. */
    static unsigned char sm[64 + 4096], out[64 + 4096];
    unsigned long long mlen = 0;
    if (len > 4096)
      return -1;
    memcpy(sm, sig, 64);
    if (len)
      memcpy(sm + 64, msg, len);
    return crypto_sign_open(out, &mlen, sm, 64 + len, pk) == 0;
  }
  case BK_COMPACT:
    return compact_ed25519_verify(sig, pk, msg, len) ? 1 : 0;
  default:
    return -1;
  }
}

/* The corpus is four colon-terminated hex fields per vector, preceded by the
 * label comment that says what the vector is for. */
static void run_corpus(void) {
  FILE *f = fopen(BOOT_VECTORS_PATH, "r");
  if (f == NULL) {
    vectors_read = -1;
    return;
  }
  static char line[1 << 16];
  char label[96] = "";
  char *fields[4] = {0};
  int nf = 0;

  while (fgets(line, sizeof line, f) != NULL) {
    char *q = line;
    while (*q == ' ' || *q == '\t')
      q++;
    if (*q == '#') { /* a label, and a comment URL also has a colon in it */
      char *e = q + 1;
      while (*e == ' ')
        e++;
      size_t L = strcspn(e, "\r\n");
      if (L != 0u && L < sizeof label - 1) {
        memcpy(label, e, L);
        label[L] = 0;
      }
      continue;
    }
    if (*q == '\n' || *q == 0)
      continue;
    char *p = strchr(line, ':');
    if (p == NULL)
      continue;
    *p = 0;
    fields[nf] = strdup(line);
    if (++nf < 4)
      continue;

    static unsigned char pk[32], sig[64], msg[4096];
    size_t pkn, sign, msgn;
    if (unhex(fields[0], pk, sizeof pk, &pkn) == 0 &&
        unhex(fields[2], sig, sizeof sig, &sign) == 0 &&
        unhex(fields[1], msg, sizeof msg, &msgn) == 0 && pkn == 32 && sign == 64) {
      vectors_read++;
      int is_valid = (strcmp(label, "valid") == 0);
      int is_invalid = (strcmp(label, "invalid") == 0);
      int is_mall = (strcmp(label, "SignatureMalleability") == 0);
      for (int b = 0; b < BK_COUNT; b++) {
        int a = accepts(b, sig, pk, msg, msgn);
        if (is_valid) {
          c[b].valid_total++;
          if (a == 1)
            c[b].valid_accepted++;
        } else if (is_invalid) {
          c[b].invalid_total++;
          if (a == 1)
            c[b].invalid_accepted++;
        } else if (is_mall) {
          c[b].malleable_total++;
          if (a == 1)
            c[b].malleable_accepted++;
        }
      }
    }
    for (int i = 0; i < 4; i++) {
      free(fields[i]);
      fields[i] = NULL;
    }
    nf = 0;
  }
  fclose(f);
}

static void ensure_loaded(void) {
  static int done = 0;
  if (!done) {
    run_corpus();
    done = 1;
  }
}

void test_boot_crypto_corpus_was_found(void) {
  ensure_loaded();
  /* A silently absent corpus would make every assertion below vacuous. */
  TEST_ASSERT_TRUE(vectors_read > 100);
}

void test_boot_crypto_every_backend_accepts_every_valid_signature(void) {
  ensure_loaded();
  for (int b = 0; b < BK_COUNT; b++) {
    TEST_ASSERT_TRUE(c[b].valid_total > 0);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)c[b].valid_total,
                             (uint32_t)c[b].valid_accepted);
  }
}

void test_boot_crypto_every_backend_rejects_every_invalid_signature(void) {
  ensure_loaded();
  for (int b = 0; b < BK_COUNT; b++) {
    TEST_ASSERT_TRUE(c[b].invalid_total > 0);
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)c[b].invalid_accepted);
  }
}

/* Pinned, not required to agree: this is the measured difference the Kconfig
 * help tells a reader about, and it should not drift unnoticed. */
void test_boot_crypto_malleability_is_what_kconfig_claims(void) {
  ensure_loaded();
  TEST_ASSERT_TRUE(c[BK_MONO].malleable_total == 8);
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)c[BK_MONO].malleable_accepted);
  TEST_ASSERT_EQUAL_UINT32(4u, (uint32_t)c[BK_TWEET].malleable_accepted);
  TEST_ASSERT_EQUAL_UINT32(4u, (uint32_t)c[BK_COMPACT].malleable_accepted);
}

/* The image digest, through the entry point stage-1 calls, against the FIPS
 * 180-4 known answers. It is one implementation for every backend now, so what
 * matters is that it is the right SHA-256 -- not that three of them agree. */
static int digest_is(const char *msg, size_t len, const char *want_hex) {
  uint8_t out[HAL_BOOT_DIGEST_SIZE];
  char got[2 * HAL_BOOT_DIGEST_SIZE + 1];
  if (hal_boot_hash(out, (const uint8_t *)msg, len) != HAL_OK)
    return 0;
  for (size_t i = 0; i < sizeof out; i++)
    sprintf(got + 2 * i, "%02x", out[i]);
  return strcmp(got, want_hex) == 0;
}

void test_boot_crypto_digest_matches_fips_180_4(void) {
  TEST_ASSERT_EQUAL_UINT32(32u, (uint32_t)HAL_BOOT_DIGEST_SIZE);
  TEST_ASSERT_TRUE(digest_is(
      "", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  TEST_ASSERT_TRUE(digest_is(
      "abc", 3,
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
  TEST_ASSERT_TRUE(digest_is(
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
      "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
}

/* A million 'a' is the vector that catches a broken length or block boundary,
 * which the short ones pass straight over. */
void test_boot_crypto_digest_handles_a_long_message(void) {
  static char million[1000000];
  memset(million, 'a', sizeof million);
  TEST_ASSERT_TRUE(digest_is(
      million, sizeof million,
      "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

void test_boot_crypto_hash_rejects_a_null_out(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_boot_hash(NULL, (const uint8_t *)"x", 1));
}

NAVTEST_CASE_DECL(test_boot_crypto_corpus_was_found);
NAVTEST_CASE_DECL(test_boot_crypto_every_backend_accepts_every_valid_signature);
NAVTEST_CASE_DECL(test_boot_crypto_every_backend_rejects_every_invalid_signature);
NAVTEST_CASE_DECL(test_boot_crypto_malleability_is_what_kconfig_claims);
NAVTEST_CASE_DECL(test_boot_crypto_digest_matches_fips_180_4);
NAVTEST_CASE_DECL(test_boot_crypto_digest_handles_a_long_message);
NAVTEST_CASE_DECL(test_boot_crypto_hash_rejects_a_null_out);

static const navtest_case_t _cases[] = {
    NAVTEST_CASE(test_boot_crypto_corpus_was_found),
    NAVTEST_CASE(test_boot_crypto_every_backend_accepts_every_valid_signature),
    NAVTEST_CASE(test_boot_crypto_every_backend_rejects_every_invalid_signature),
    NAVTEST_CASE(test_boot_crypto_malleability_is_what_kconfig_claims),
    NAVTEST_CASE(test_boot_crypto_digest_matches_fips_180_4),
    NAVTEST_CASE(test_boot_crypto_digest_handles_a_long_message),
    NAVTEST_CASE(test_boot_crypto_hash_rejects_a_null_out),
};

const navtest_suite_t test_boot_crypto_suite = {
    .name = "boot_crypto",
    .cases = _cases,
    .count = sizeof _cases / sizeof _cases[0],
};
