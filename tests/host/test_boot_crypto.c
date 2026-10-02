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

/* The three SHA-512s have to agree, or the image digest depends on which backend
 * was configured -- and a signed image would stop verifying on a reconfigure. */
void test_boot_crypto_the_three_hashes_agree(void) {
  static const unsigned char msg[77] =
      "the image digest must not depend on which backend was configured today";
  unsigned char a[64], b[64], d[64];

  crypto_sha512(a, msg, sizeof msg);
  crypto_hash(b, msg, sizeof msg);

  struct sha512_state st;
  sha512_init(&st);
  size_t off = 0;
  while (sizeof msg - off >= SHA512_BLOCK_SIZE) {
    sha512_block(&st, msg + off);
    off += SHA512_BLOCK_SIZE;
  }
  sha512_final(&st, msg + off, sizeof msg);
  sha512_get(&st, d, 0, 64);

  /* navtest has no memory-compare assertion, and a byte-wise loop reports which
   * byte differs rather than just that something did. */
  for (int i = 0; i < 64; i++) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)a[i], (uint32_t)b[i]);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)a[i], (uint32_t)d[i]);
  }
}

NAVTEST_CASE_DECL(test_boot_crypto_corpus_was_found);
NAVTEST_CASE_DECL(test_boot_crypto_every_backend_accepts_every_valid_signature);
NAVTEST_CASE_DECL(test_boot_crypto_every_backend_rejects_every_invalid_signature);
NAVTEST_CASE_DECL(test_boot_crypto_malleability_is_what_kconfig_claims);
NAVTEST_CASE_DECL(test_boot_crypto_the_three_hashes_agree);

static const navtest_case_t _cases[] = {
    NAVTEST_CASE(test_boot_crypto_corpus_was_found),
    NAVTEST_CASE(test_boot_crypto_every_backend_accepts_every_valid_signature),
    NAVTEST_CASE(test_boot_crypto_every_backend_rejects_every_invalid_signature),
    NAVTEST_CASE(test_boot_crypto_malleability_is_what_kconfig_claims),
    NAVTEST_CASE(test_boot_crypto_the_three_hashes_agree),
};

const navtest_suite_t test_boot_crypto_suite = {
    .name = "boot_crypto",
    .cases = _cases,
    .count = sizeof _cases / sizeof _cases[0],
};
