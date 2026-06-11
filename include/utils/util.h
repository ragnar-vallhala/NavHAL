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

#ifndef HAL_UTIL_H
#define HAL_UTIL_H
/**
 * @defgroup HAL_UTIL_GENERIC Util Generic
 * @ingroup HAL_UTILS
 * @brief Miscellaneous utility macros.
 * @{
 */

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
void hal_memcpy(void *dest, const void *src, uint32_t len);
void hal_memset(void *dest, int val, uint32_t len);
int hal_memcmp(const void *s1, const void *s2, uint32_t len);
uint32_t hal_strlen(const char *s);
char *hal_strchr(const char *s, int c);


#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_UTIL_GENERIC */
#endif // !HAL_UTIL_H