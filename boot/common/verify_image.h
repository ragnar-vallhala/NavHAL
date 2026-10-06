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
 * @file verify_image.h
 * @brief One verifier: both stages, both boot paths, and the update server.
 *
 * Shared on purpose. If recovery reported success by a different rule than the
 * one deciding whether the board boots, a host could be told an image is good
 * and then watch the board refuse it -- or worse, the reverse.
 */
#ifndef BOOT_VERIFY_IMAGE_H
#define BOOT_VERIFY_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Is the image at @p base signed by the key this unit carries?
 *
 * @param base     Partition base: the image header, not the payload.
 * @param max_body Largest body the partition can hold, used to bound the
 *                 header's length field before it bounds the hash.
 * @return true only when the magic, the length, the digest and the signature all
 *         agree.
 */
bool boot_image_is_good(uint32_t base, uint32_t max_body);

#endif /* BOOT_VERIFY_IMAGE_H */
