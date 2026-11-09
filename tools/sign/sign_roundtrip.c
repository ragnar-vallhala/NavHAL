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
 * @file sign_roundtrip.c
 * @brief Does what the tool produced verify the way stage-1 will verify it?
 *
 * Runs navhal_sign over a body of pseudo-random bytes, then checks the result
 * through hal_boot_hash and hal_boot_ed25519_verify -- the firmware's own
 * functions, linked here rather than reimplemented, because a check that agrees
 * with itself proves nothing.
 *
 * Then it flips one byte of the body and requires the verify to fail. A signing
 * tool that produced an image which verified before and after tampering would
 * pass every other test in this repo.
 */
#include "common/hal_bootmap.h"
#include "common/hal_boot_crypto.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BODY_LEN 4096u

static int fail(const char *what) {
  printf("  FAIL %s\n", what);
  return 1;
}

/* The verifier, written the way stage-1 will: clamp length first, hash the
 * signed prefix then the body, check the signature over that digest. */
static int verify_image(const uint8_t *image, size_t image_len, uint32_t max_body,
                        const uint8_t *pk) {
  if (image_len < HAL_BOOTMAP_HEADER_SIZE)
    return 0;
  const hal_boot_image_header_t *h = (const hal_boot_image_header_t *)image;
  if (h->magic != (uint32_t)HAL_BOOTMAP_IMAGE_MAGIC)
    return 0;
  if (h->length == 0u || h->length > max_body)
    return 0; /* clamped before it bounds the hash */
  if (image_len < HAL_BOOTMAP_HEADER_SIZE + h->length)
    return 0;

  uint8_t digest[HAL_BOOT_DIGEST_SIZE];
  uint8_t *buf = malloc(HAL_BOOTMAP_SIGNED_PREFIX + h->length);
  memcpy(buf, image, HAL_BOOTMAP_SIGNED_PREFIX);
  memcpy(buf + HAL_BOOTMAP_SIGNED_PREFIX, image + HAL_BOOTMAP_HEADER_SIZE,
         h->length);
  int ok = hal_boot_hash(digest, buf, HAL_BOOTMAP_SIGNED_PREFIX + h->length) ==
           HAL_OK;
  free(buf);
  if (!ok)
    return 0;
  if (memcmp(digest, h->digest, sizeof digest) != 0)
    return 0; /* the header's digest is not the image's */
  return hal_boot_ed25519_verify(h->sig, pk, digest, sizeof digest) == HAL_OK;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: sign_roundtrip <path to navhal_sign>\n");
    return 2;
  }
  const char *tool = argv[1];
  int failures = 0;

  /* A body that is not all one value, so a hash that ignores most of it still
   * has to get the bytes right. */
  uint8_t body[BODY_LEN];
  for (unsigned i = 0; i < BODY_LEN; i++)
    body[i] = (uint8_t)((i * 31u + (i >> 3)) & 0xFFu);
  FILE *f = fopen("/tmp/navhal_body.bin", "wb");
  fwrite(body, 1, sizeof body, f);
  fclose(f);

  char cmd[1024];
  snprintf(cmd, sizeof cmd,
           "%s --genkey --key /tmp/navhal_k.sec --pub /tmp/navhal_k.pub >/dev/null",
           tool);
  if (system(cmd) != 0)
    return fail("could not generate a key");

  snprintf(cmd, sizeof cmd,
           "%s --partition app --key /tmp/navhal_k.sec --version 7 "
           "--in /tmp/navhal_body.bin --out /tmp/navhal_image.bin >/dev/null",
           tool);
  if (system(cmd) != 0)
    return fail("could not sign");

  uint8_t pk[32];
  f = fopen("/tmp/navhal_k.pub", "rb");
  if (!f || fread(pk, 1, sizeof pk, f) != sizeof pk)
    return fail("could not read the public key");
  fclose(f);

  f = fopen("/tmp/navhal_image.bin", "rb");
  if (!f)
    return fail("could not read the image");
  static uint8_t image[HAL_BOOTMAP_HEADER_SIZE + BODY_LEN];
  size_t n = fread(image, 1, sizeof image, f);
  fclose(f);

  if (n != HAL_BOOTMAP_HEADER_SIZE + BODY_LEN)
    failures += fail("image is not header + body in size");
  else
    printf("  ok   image is %zu bytes: a %u-byte header and a %u-byte body\n", n,
           (unsigned)HAL_BOOTMAP_HEADER_SIZE, BODY_LEN);

  const hal_boot_image_header_t *h = (const hal_boot_image_header_t *)image;
  if (h->version != 7u)
    failures += fail("version did not survive into the header");
  else
    printf("  ok   version 7 is in the header\n");

  if (verify_image(image, n, HAL_BOOTMAP_APP_USABLE, pk))
    printf("  ok   the firmware's verifier accepts what the tool signed\n");
  else
    failures += fail("the firmware's verifier rejected a good image");

  /* One byte of the body, well past the header. */
  image[HAL_BOOTMAP_HEADER_SIZE + 1234u] ^= 0x01u;
  if (!verify_image(image, n, HAL_BOOTMAP_APP_USABLE, pk))
    printf("  ok   one flipped body byte is rejected\n");
  else
    failures += fail("a tampered body still verified");
  image[HAL_BOOTMAP_HEADER_SIZE + 1234u] ^= 0x01u;

  /* And the header's own fields are covered: version is inside the digest. */
  ((hal_boot_image_header_t *)image)->version = 8u;
  if (!verify_image(image, n, HAL_BOOTMAP_APP_USABLE, pk))
    printf("  ok   editing version is rejected -- the digest covers it\n");
  else
    failures += fail("version could be edited without breaking the signature");

  printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
  return failures ? 1 : 0;
}
