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
 * @file utils/freestanding.c
 * @brief The four library functions a freestanding C implementation may still
 *        require the program to supply.
 *
 * @details
 * Above @c -O0 GCC emits @c memcpy / @c memset for an aggregate assignment or
 * a zero-initialised struct return -- C11 4.6 lets a freestanding
 * implementation require exactly these -- and with @c -nostdlib nothing else
 * defines them. Because these definitions win over newlib's in any image that
 * links NavHAL, every kernel and driver copy runs through them, so they move
 * a word at a time, as newlib does:
 *  - align the destination with byte stores, then copy words, four per
 *    iteration where the source is aligned too;
 *  - a misaligned source is read a word at a time where the core allows it
 *    (ARMv7-M with CCR.UNALIGN_TRP clear, which NavHAL never sets; x86), and
 *    byte by byte elsewhere;
 *  - short tails and copies under two words stay byte loops;
 *  - AVR keeps byte loops throughout: a byte is its native width.
 *
 * On ARMv7-M / ARMv7E-M memcpy is newlib's hand-scheduled assembly instead
 * (@c memcpy-armv7m.S beside this file), which copies 64-byte blocks.
 *
 * These are the definitions, not wrappers: @c hal_memcpy and friends in
 * @c util.c forward here. Each body is built with loop-pattern distribution
 * off, so GCC can never turn a loop in it into a call to itself.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) && !defined(__clang__)
#define NO_LIBCALL __attribute__((optimize("no-tree-loop-distribute-patterns")))
#else
#define NO_LIBCALL
#endif

#if defined(__AVR__)
#define WORD_COPY 0
#else
#define WORD_COPY 1
#endif

#if defined(__ARM_FEATURE_UNALIGNED) || defined(__x86_64__) || defined(__i386__)
#define UNALIGNED_OK 1
#else
#define UNALIGNED_OK 0
#endif

typedef uint32_t word_t;
typedef uint32_t __attribute__((may_alias)) aword_t; /* an aligned word */
typedef uint32_t __attribute__((may_alias, aligned(1))) uword_t; /* any address */
#define W sizeof(word_t)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#define ASM_MEMCPY 1 /* memcpy-armv7m.S */
#else
#define ASM_MEMCPY 0
#endif
#define MIS(p) ((uintptr_t)(p) & (W - 1u))

#if !ASM_MEMCPY
/* memmove relies on this copying forwards in blocks that are read before they
 * are written, which makes it safe for an overlap with dest below src. */
NO_LIBCALL void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
#if WORD_COPY
  if (n >= 2u * W) {
    while (MIS(d)) {
      *d++ = *s++;
      n--;
    }
    if (!MIS(s)) {
      aword_t *dw = (aword_t *)d;
      const aword_t *sw = (const aword_t *)s;
      for (; n >= 4u * W; n -= 4u * W, dw += 4, sw += 4) {
        word_t a = sw[0], b = sw[1], c = sw[2], e = sw[3];
        dw[0] = a;
        dw[1] = b;
        dw[2] = c;
        dw[3] = e;
      }
      for (; n >= W; n -= W)
        *dw++ = *sw++;
      d = (unsigned char *)dw;
      s = (const unsigned char *)sw;
    }
#if UNALIGNED_OK
    else {
      aword_t *dw = (aword_t *)d;
      const uword_t *sw = (const uword_t *)s;
      for (; n >= W; n -= W)
        *dw++ = *sw++;
      d = (unsigned char *)dw;
      s = (const unsigned char *)sw;
    }
#endif
  }
#endif
  while (n--)
    *d++ = *s++;
  return dest;
}
#else
void *memcpy(void *dest, const void *src, size_t n);
#endif

NO_LIBCALL void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s || n == 0u)
    return dest;
  if (d < s || d >= s + n)
    return memcpy(dest, src, n);

  /* dest above src and overlapping: copy downwards so the tail is read before
   * it is overwritten. */
  d += n;
  s += n;
#if WORD_COPY
  if (n >= 2u * W) {
    while (MIS(d)) {
      *--d = *--s;
      n--;
    }
    if (!MIS(s)) {
      aword_t *dw = (aword_t *)d;
      const aword_t *sw = (const aword_t *)s;
      for (; n >= W; n -= W)
        *--dw = *--sw;
      d = (unsigned char *)dw;
      s = (const unsigned char *)sw;
    }
#if UNALIGNED_OK
    else {
      aword_t *dw = (aword_t *)d;
      const uword_t *sw = (const uword_t *)s;
      for (; n >= W; n -= W)
        *--dw = *--sw;
      d = (unsigned char *)dw;
      s = (const unsigned char *)sw;
    }
#endif
  }
#endif
  while (n--)
    *--d = *--s;
  return dest;
}

NO_LIBCALL void *memset(void *dest, int c, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  unsigned char b = (unsigned char)c;
#if WORD_COPY
  if (n >= 2u * W) {
    while (MIS(d)) {
      *d++ = b;
      n--;
    }
    word_t w = b * (word_t)0x01010101u;
    aword_t *dw = (aword_t *)d;
    for (; n >= 4u * W; n -= 4u * W, dw += 4) {
      dw[0] = w;
      dw[1] = w;
      dw[2] = w;
      dw[3] = w;
    }
    for (; n >= W; n -= W)
      *dw++ = w;
    d = (unsigned char *)dw;
  }
#endif
  while (n--)
    *d++ = b;
  return dest;
}

NO_LIBCALL int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = (const unsigned char *)s1;
  const unsigned char *b = (const unsigned char *)s2;
#if WORD_COPY
  /* Skip equal words; the byte loop below finds which byte of a differing
   * word decides the sign. */
  if (n >= 2u * W && MIS(a) == MIS(b)) {
    for (; MIS(a); a++, b++, n--)
      if (*a != *b)
        return (int)*a - (int)*b;
    for (; n >= W; n -= W, a += W, b += W)
      if (*(const aword_t *)a != *(const aword_t *)b)
        break;
  }
#endif
  for (; n; n--, a++, b++)
    if (*a != *b)
      return (int)*a - (int)*b;
  return 0;
}
