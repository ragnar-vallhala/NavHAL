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

#include "utils/util.h"
#include <stddef.h>

// The word-at-a-time definitions in freestanding.c; declared here because a
// freestanding build has no <string.h>.
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void hal_memcpy(void *dest, const void *src, uint32_t len) {
  memcpy(dest, src, len);
}
void hal_memset(void *dest, int val, uint32_t len) { memset(dest, val, len); }

int hal_memcmp(const void *s1, const void *s2, uint32_t len) {
  return memcmp(s1, s2, len);
}

uint32_t hal_strlen(const char *s) {
  uint32_t len = 0;
  while (*s++)
    len++;
  return len;
}

char *hal_strchr(const char *s, int c) {
  while (*s != (char)c) {
    if (!*s)
      return 0;
    s++;
  }
  return (char *)s;
}
