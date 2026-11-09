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

#include "compact_wipe.h"
#include <stdlib.h>
#include <string.h>

void *compact_wipe(void *data, size_t length) {
// simplification of: https://www.cryptologie.net/article/419/zeroing-memory-compiler-optimizations-and-memset_s/
	 volatile unsigned char *p = data;
	 while (length--){
		 *p++ = 0;
	 }
	return data;
}
