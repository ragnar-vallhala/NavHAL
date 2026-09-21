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
 * @c -ffreestanding stops GCC rewriting an ordinary loop into a library call,
 * but it does not stop it emitting @c memcpy / @c memset for an aggregate
 * assignment or a zero-initialised struct return -- C11 4.6 lets a
 * freestanding implementation require exactly these. With @c -nostdlib there
 * is no libc to satisfy them, so the link fails at any optimisation level
 * above @c -O0 unless the program defines them itself.
 *
 * These are the definitions, not wrappers: @c hal_memcpy and friends in
 * @c util.c are the API NavHAL code calls, while these exist for the compiler.
 * Deliberately plain byte loops -- correctness over speed, and a naive body is
 * also what keeps GCC from recognising the pattern and emitting a call to the
 * function it is compiling.
 */

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--)
    *d++ = *s++;
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s || n == 0u)
    return dest;

  /* Overlapping and dest above src: copy downwards so the tail is read before
   * it is overwritten. */
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }
  return dest;
}

void *memset(void *dest, int c, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  while (n--)
    *d++ = (unsigned char)c;
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = (const unsigned char *)s1;
  const unsigned char *b = (const unsigned char *)s2;
  while (n--) {
    if (*a != *b)
      return (int)*a - (int)*b;
    a++;
    b++;
  }
  return 0;
}
