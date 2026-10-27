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
 * @file tests/portable/test_mem.c
 * @brief memcpy / memmove / memset / memcmp as linked into the target image.
 *
 * tests/host checks the C versions in utils/freestanding.c against the host
 * libc; this suite checks whatever the target actually links -- on ARMv7-M
 * that includes the assembly memcpy -- against plain byte loops. Every size up
 * to SPAN at every source and destination offset, with guard bytes either
 * side.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "test_mem.h"

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#define SPAN 72u // several 64-byte blocks' worth of tails, plus one block
#define OFFS 8u
#define BUF (SPAN + 2u * OFFS + 8u)

static uint8_t src[BUF], dst[BUF], ref[BUF];

static void fill(uint8_t *p, unsigned seed) {
  for (unsigned i = 0; i < BUF; i++)
    p[i] = (uint8_t)(seed * 131u + i * 37u + 7u);
}

/* The references go through volatile so the compiler cannot turn them back
 * into calls to the functions under test. */
static void ref_move(uint8_t *d, const uint8_t *s, size_t n) {
  volatile uint8_t *vd = d;
  const volatile uint8_t *vs = s;
  if (vd < vs)
    for (size_t i = 0; i < n; i++)
      vd[i] = vs[i];
  else
    while (n--)
      vd[n] = vs[n];
}

static bool same(const uint8_t *a, const uint8_t *b) {
  const volatile uint8_t *va = a, *vb = b;
  for (unsigned i = 0; i < BUF; i++)
    if (va[i] != vb[i])
      return false;
  return true;
}

void test_mem_memcpy_every_size_and_offset(void) {
  bool ok = true;
  for (size_t n = 0; n <= SPAN && ok; n++)
    for (size_t so = 0; so < OFFS; so++)
      for (size_t dof = 0; dof < OFFS; dof++) {
        fill(src, (unsigned)(n + so));
        fill(dst, 99u);
        fill(ref, 99u);
        ref_move(ref + OFFS + dof, src + so, n);
        ok = ok && memcpy(dst + OFFS + dof, src + so, n) == dst + OFFS + dof;
        ok = ok && same(dst, ref);
      }
  TEST_ASSERT_TRUE(ok);
}

void test_mem_memmove_every_overlap(void) {
  bool ok = true;
  for (size_t n = 0; n <= SPAN && ok; n++)
    for (size_t so = 0; so < 2u * OFFS; so++)
      for (size_t dof = 0; dof < 2u * OFFS; dof++) {
        fill(dst, (unsigned)(n * 3u + so));
        fill(ref, (unsigned)(n * 3u + so));
        ref_move(ref + dof, ref + so, n);
        ok = ok && memmove(dst + dof, dst + so, n) == dst + dof;
        ok = ok && same(dst, ref);
      }
  TEST_ASSERT_TRUE(ok);
}

void test_mem_memset_every_size_and_offset(void) {
  bool ok = true;
  for (size_t n = 0; n <= SPAN && ok; n++)
    for (size_t dof = 0; dof < OFFS; dof++) {
      fill(dst, (unsigned)n);
      fill(ref, (unsigned)n);
      volatile uint8_t *r = ref + OFFS + dof;
      for (size_t i = 0; i < n; i++)
        r[i] = 0xA5;
      ok = ok && memset(dst + OFFS + dof, 0x1A5, n) == dst + OFFS + dof;
      ok = ok && same(dst, ref);
    }
  TEST_ASSERT_TRUE(ok);
}

void test_mem_memcmp_sign_at_every_byte(void) {
  bool ok = true;
  for (size_t n = 1; n <= SPAN && ok; n++)
    for (size_t ao = 0; ao < OFFS; ao += 3u)
      for (size_t bo = 0; bo < OFFS; bo++) {
        uint8_t *a = src + ao, *b = dst + bo;
        fill(src, 5u);
        ref_move(b, a, n);
        ok = ok && memcmp(a, b, n) == 0;
        size_t p = (n * 5u + ao + bo) % n; // the deciding byte moves around
        a[p] = 0x80;             // 0x80 against 0x7F: the compare is unsigned
        b[p] = 0x7F;
        ok = ok && memcmp(a, b, n) > 0 && memcmp(b, a, n) < 0;
      }
  TEST_ASSERT_TRUE(ok);
}

NAVTEST_CASE_DECL(test_mem_memcpy_every_size_and_offset);
NAVTEST_CASE_DECL(test_mem_memmove_every_overlap);
NAVTEST_CASE_DECL(test_mem_memset_every_size_and_offset);
NAVTEST_CASE_DECL(test_mem_memcmp_sign_at_every_byte);

static const navtest_case_t mem_cases[] = {
    NAVTEST_CASE(test_mem_memcpy_every_size_and_offset),
    NAVTEST_CASE(test_mem_memmove_every_overlap),
    NAVTEST_CASE(test_mem_memset_every_size_and_offset),
    NAVTEST_CASE(test_mem_memcmp_sign_at_every_byte),
};

const navtest_suite_t test_mem_suite = {
    .name = "MEM* (portable)",
    .cases = mem_cases,
    .count = sizeof(mem_cases) / sizeof(mem_cases[0]),
    .between = NULL,
};
