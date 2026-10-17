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

#ifndef __COMPACT_WIPE_H
#define __COMPACT_WIPE_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Try to wipe contents of a buffer, as best as we can.
    (memset can be ignored by the compiler)

    Use this to clear private key data if you do not need it anymore.

    returns the data pointer, makes for easier chaining
*/
void *compact_wipe(void *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
