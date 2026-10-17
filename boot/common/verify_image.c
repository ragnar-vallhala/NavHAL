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
 * @file verify_image.c
 * @brief The image check, in one place so recovery and the boot path agree.
 */
#include "verify_image.h"

#include "common/hal_boot_crypto.h"
#include "common/hal_bootmap.h"
#include "boot_pubkey.h"

#include <string.h>

/* Verify the image at @p base against @p max_body. The order matters: magic,
 * then the length bound, then the hash, then the signature. Hashing before the
 * length is checked is how a verifier gets walked off the end of flash into
 * bytes an attacker chose. */
bool boot_image_is_good(uint32_t base, uint32_t max_body) {
  const hal_boot_image_header_t *h = (const hal_boot_image_header_t *)base;

  if (h->magic != (uint32_t)HAL_BOOTMAP_IMAGE_MAGIC)
    return false;
  if (h->length == 0u || h->length > max_body)
    return false;

  /* The digest covers the header's first 12 bytes and then the body, which are
   * not contiguous -- the padding sits between them -- so the hash is fed in two
   * parts rather than over one range. */
  uint8_t digest[HAL_BOOT_DIGEST_SIZE];
  if (hal_boot_hash_split(digest, (const uint8_t *)base,
                          HAL_BOOTMAP_SIGNED_PREFIX,
                          (const uint8_t *)(base + HAL_BOOTMAP_HEADER_SIZE),
                          h->length) != HAL_OK)
    return false;

  if (memcmp(digest, h->digest, sizeof digest) != 0)
    return false;

  return hal_boot_ed25519_verify(h->sig, boot_pubkey, digest,
                                 sizeof digest) == HAL_OK;
}
