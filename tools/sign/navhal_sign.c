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
 * @file navhal_sign.c
 * @brief Prepend the image header, hash the image, sign the hash.
 *
 * Built on the same vendored Monocypher that verifies on target, so host and
 * device cannot disagree about what a valid signature is -- and the Wycheproof
 * corpus the host suite runs covers both sides of the signature rather than only
 * the verifier.
 *
 * The private key never leaves the host. --genkey writes a keypair; the public
 * half is what stage-1 carries.
 *
 *   navhal_sign --genkey --key k.sec --pub k.pub
 *   navhal_sign --partition stage2|app --key k.sec --version N \
 *               --in body.bin --out image.bin
 *
 * --partition is explicit rather than inferred from the file's size, because the
 * two have different maxima and signing a stage-2 image against the app's bound
 * would produce something stage-1 accepts into a partition it does not fit.
 */
#include "common/hal_bootmap.h"
#include "monocypher-ed25519.h"
#include "sha256.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The partition maxima this tool clamps against, from hal_bootmap.h. */
struct partition {
  const char *name;
  uint32_t usable;
};
static const struct partition partitions[] = {
    {"stage2", HAL_BOOTMAP_STAGE2_USABLE},
    {"app", HAL_BOOTMAP_APP_USABLE},
};

static int die(const char *msg) {
  fprintf(stderr, "navhal_sign: %s\n", msg);
  return 1;
}

static int read_file(const char *path, uint8_t **out, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return -1;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) {
    fclose(f);
    return -1;
  }
  *out = malloc((size_t)n ? (size_t)n : 1);
  *len = fread(*out, 1, (size_t)n, f);
  fclose(f);
  return (*len == (size_t)n) ? 0 : -1;
}

static int write_file(const char *path, const uint8_t *data, size_t len) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t w = fwrite(data, 1, len, f);
  fclose(f);
  return (w == len) ? 0 : -1;
}

static int genkey(const char *sec_path, const char *pub_path) {
  uint8_t seed[32], sk[64], pk[32];
  FILE *r = fopen("/dev/urandom", "rb");
  if (!r || fread(seed, 1, sizeof seed, r) != sizeof seed)
    return die("cannot read /dev/urandom");
  fclose(r);

  crypto_ed25519_key_pair(sk, pk, seed); /* wipes seed */
  if (write_file(sec_path, sk, sizeof sk) != 0)
    return die("cannot write the secret key");
  if (write_file(pub_path, pk, sizeof pk) != 0)
    return die("cannot write the public key");
  crypto_wipe(sk, sizeof sk);

  printf("wrote %s (64 bytes, keep it) and %s (32 bytes, goes in stage-1)\n",
         sec_path, pub_path);
  return 0;
}

int main(int argc, char **argv) {
  const char *key = NULL, *pub = NULL, *in = NULL, *out = NULL, *part = NULL;
  unsigned long version = 0;
  int do_genkey = 0;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--genkey"))
      do_genkey = 1;
    else if (!strcmp(argv[i], "--key") && i + 1 < argc)
      key = argv[++i];
    else if (!strcmp(argv[i], "--pub") && i + 1 < argc)
      pub = argv[++i];
    else if (!strcmp(argv[i], "--in") && i + 1 < argc)
      in = argv[++i];
    else if (!strcmp(argv[i], "--out") && i + 1 < argc)
      out = argv[++i];
    else if (!strcmp(argv[i], "--partition") && i + 1 < argc)
      part = argv[++i];
    else if (!strcmp(argv[i], "--version") && i + 1 < argc)
      version = strtoul(argv[++i], NULL, 0);
    else
      return die("unknown argument; see the comment at the top of this file");
  }

  if (do_genkey)
    return (key && pub) ? genkey(key, pub) : die("--genkey needs --key and --pub");
  if (!key || !in || !out || !part)
    return die("need --partition, --key, --in and --out");

  const struct partition *p = NULL;
  for (size_t i = 0; i < sizeof partitions / sizeof partitions[0]; i++)
    if (!strcmp(partitions[i].name, part))
      p = &partitions[i];
  if (!p)
    return die("--partition must be stage2 or app");

  uint8_t *body = NULL, *sk = NULL;
  size_t body_len = 0, sk_len = 0;
  if (read_file(in, &body, &body_len) != 0)
    return die("cannot read --in");
  if (read_file(key, &sk, &sk_len) != 0 || sk_len != 64)
    return die("--key must be a 64-byte secret key from --genkey");

  /* Refused here rather than clamped: a body that does not fit is a build
   * problem, and silently truncating it would produce an image that verifies
   * and then jumps into nothing. */
  if (body_len == 0)
    return die("--in is empty");
  if (body_len > p->usable) {
    fprintf(stderr, "navhal_sign: body is %zu bytes, %s holds %u\n", body_len,
            p->name, (unsigned)p->usable);
    return 1;
  }

  hal_boot_image_header_t h;
  memset(&h, 0xFF, sizeof h); /* pad as erased flash, not as zeros */
  h.magic = (uint32_t)HAL_BOOTMAP_IMAGE_MAGIC;
  h.version = (uint32_t)version;
  h.length = (uint32_t)body_len;

  /* The digest covers the first 12 bytes of the header, then the body -- the
   * same order and extent the verifier uses. */
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, (const uint8_t *)&h, HAL_BOOTMAP_SIGNED_PREFIX);
  sha256_update(&ctx, body, body_len);
  sha256_final(&ctx, h.digest);

  crypto_ed25519_sign(h.sig, sk, h.digest, sizeof h.digest);
  crypto_wipe(sk, sk_len);

  uint8_t *image = malloc(sizeof h + body_len);
  memcpy(image, &h, sizeof h);
  memcpy(image + sizeof h, body, body_len);
  if (write_file(out, image, sizeof h + body_len) != 0)
    return die("cannot write --out");

  printf("%s: %s, version %lu, body %zu bytes, image %zu bytes\n", out, p->name,
         version, body_len, sizeof h + body_len);
  printf("  sha256 ");
  for (size_t i = 0; i < 32; i++)
    printf("%02x", h.digest[i]);
  printf("\n");
  return 0;
}
