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
 * @file tests/host/test_freestanding.c
 * @brief src/utils/freestanding.c against the host libc.
 *
 * The file is compiled in here under other names, so the host's own memcpy
 * stays the reference. Every size up to SPAN at every source and destination
 * offset covers the byte head, both word paths (aligned and misaligned
 * source) and the byte tail; guard bytes either side catch a write outside
 * the range.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "test_freestanding.h"

#define memcpy fs_memcpy
#define memmove fs_memmove
#define memset fs_memset
#define memcmp fs_memcmp
#include "../../src/utils/freestanding.c"
#undef memcpy
#undef memmove
#undef memset
#undef memcmp

#define SPAN 80u // sizes 0..SPAN: several 4-word blocks plus every tail
#define OFFS 8u  // offsets 0..7 on each side
#define BUF (SPAN + 2u * OFFS + 16u)

static unsigned char src[BUF], dst[BUF], ref[BUF];

static void fill(unsigned char *p, size_t n, unsigned seed) {
  for (size_t i = 0; i < n; i++)
    p[i] = (unsigned char)(seed * 131u + i * 37u + 7u);
}

void test_freestanding_memcpy_every_size_and_offset(void) {
  for (size_t n = 0; n <= SPAN; n++)
    for (size_t so = 0; so < OFFS; so++)
      for (size_t dof = 0; dof < OFFS; dof++) {
        fill(src, BUF, (unsigned)(n + so));
        fill(dst, BUF, 99u);
        memcpy(ref, dst, BUF);
        memcpy(ref + 8 + dof, src + so, n);
        void *r = fs_memcpy(dst + 8 + dof, src + so, n);
        TEST_ASSERT_TRUE(r == dst + 8 + dof);
        TEST_ASSERT_EQUAL_UINT32(0, memcmp(dst, ref, BUF));
      }
}

void test_freestanding_memmove_every_overlap(void) {
  // Source and destination both inside one buffer, every pair of offsets up
  // to 2*OFFS apart in either direction, so every overlap shape is hit.
  for (size_t n = 0; n <= SPAN; n++)
    for (size_t so = 0; so < 2u * OFFS; so++)
      for (size_t dof = 0; dof < 2u * OFFS; dof++) {
        fill(dst, BUF, (unsigned)(n * 3u + so));
        memcpy(ref, dst, BUF);
        memmove(ref + dof, ref + so, n);
        void *r = fs_memmove(dst + dof, dst + so, n);
        TEST_ASSERT_TRUE(r == dst + dof);
        TEST_ASSERT_EQUAL_UINT32(0, memcmp(dst, ref, BUF));
      }
}

void test_freestanding_memset_every_size_and_offset(void) {
  static const int vals[] = {0, 0x5A, 0x80, 0xFF, 0x1A5}; // 0x1A5: low byte only
  for (size_t v = 0; v < sizeof vals / sizeof vals[0]; v++)
    for (size_t n = 0; n <= SPAN; n++)
      for (size_t dof = 0; dof < OFFS; dof++) {
        fill(dst, BUF, (unsigned)n);
        memcpy(ref, dst, BUF);
        memset(ref + 8 + dof, vals[v], n);
        void *r = fs_memset(dst + 8 + dof, vals[v], n);
        TEST_ASSERT_TRUE(r == dst + 8 + dof);
        TEST_ASSERT_EQUAL_UINT32(0, memcmp(dst, ref, BUF));
      }
}

static int sign(int x) { return (x > 0) - (x < 0); }

void test_freestanding_memcmp_sign_at_every_byte(void) {
  for (size_t n = 0; n <= SPAN; n++)
    for (size_t ao = 0; ao < OFFS; ao++)
      for (size_t bo = 0; bo < OFFS; bo += 3u) { // same and differing alignment
        unsigned char *a = src + ao, *b = dst + bo;
        fill(a, n, 5u);
        memcpy(b, a, n);
        TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)fs_memcmp(a, b, n));
        // 0x80 against 0x7F: the comparison must be unsigned.
        for (size_t p = 0; p < n; p++) {
          unsigned char keep = b[p];
          a[p] = 0x80;
          b[p] = 0x7F;
          TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)sign(fs_memcmp(a, b, n)));
          TEST_ASSERT_EQUAL_UINT32((uint32_t)-1, (uint32_t)sign(fs_memcmp(b, a, n)));
          a[p] = b[p] = keep;
        }
      }
}

void test_freestanding_large_random(void) {
  static unsigned char a[4096 + 8], b[4096 + 8], c[4096 + 8];
  srand(1);
  for (int it = 0; it < 200; it++) {
    size_t n = (size_t)rand() % 4096u, ao = (size_t)rand() % 8u,
           bo = (size_t)rand() % 8u;
    for (size_t i = 0; i < sizeof a; i++)
      a[i] = (unsigned char)rand();
    memset(b, 0, sizeof b);
    memset(c, 0, sizeof c);
    fs_memcpy(b + bo, a + ao, n);
    memcpy(c + bo, a + ao, n);
    TEST_ASSERT_EQUAL_UINT32(0, memcmp(b, c, sizeof b));
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)fs_memcmp(b + bo, a + ao, n));
  }
}

NAVTEST_CASE_DECL(test_freestanding_memcpy_every_size_and_offset);
NAVTEST_CASE_DECL(test_freestanding_memmove_every_overlap);
NAVTEST_CASE_DECL(test_freestanding_memset_every_size_and_offset);
NAVTEST_CASE_DECL(test_freestanding_memcmp_sign_at_every_byte);
NAVTEST_CASE_DECL(test_freestanding_large_random);

static const navtest_case_t freestanding_cases[] = {
    NAVTEST_CASE(test_freestanding_memcpy_every_size_and_offset),
    NAVTEST_CASE(test_freestanding_memmove_every_overlap),
    NAVTEST_CASE(test_freestanding_memset_every_size_and_offset),
    NAVTEST_CASE(test_freestanding_memcmp_sign_at_every_byte),
    NAVTEST_CASE(test_freestanding_large_random),
};

const navtest_suite_t test_freestanding_suite = {
    .name = "FREESTANDING MEM* (host)",
    .cases = freestanding_cases,
    .count = sizeof(freestanding_cases) / sizeof(freestanding_cases[0]),
    .between = NULL,
};
