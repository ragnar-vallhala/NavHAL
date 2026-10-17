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
 * @file backend_tweetnacl.c
 * @brief hal_boot_crypto over TweetNaCl. Half the flash of Monocypher and 39x
 *        the time per verify; accepts 4 of the 8 malleable signatures in the
 *        Wycheproof corpus, because it does not range-check S.
 *
 * crypto_sign_open takes signature and message in one buffer and writes the
 * message back out, so a verify needs somewhere to put a copy. That buffer is
 * the reason this backend has a message-size ceiling the others do not: a
 * bootloader verifies a digest, not an image, so 64 bytes is enough and a longer
 * message is refused rather than silently truncated.
 */
#include "common/hal_boot_crypto.h"
#include "tweetnacl.h"
#include <string.h>

#define TWEET_MAX_MSG 64u

hal_status_t hal_boot_ed25519_verify(const uint8_t *sig, const uint8_t *pk,
                                    const uint8_t *msg, size_t len) {
  if (sig == NULL || pk == NULL || (msg == NULL && len != 0u))
    return HAL_ERR_INVALID_ARG;
  if (len > TWEET_MAX_MSG)
    return HAL_ERR_INVALID_ARG;

  uint8_t sm[HAL_BOOT_SIG_SIZE + TWEET_MAX_MSG];
  uint8_t out[HAL_BOOT_SIG_SIZE + TWEET_MAX_MSG];
  unsigned long long mlen = 0;

  memcpy(sm, sig, HAL_BOOT_SIG_SIZE);
  if (len != 0u)
    memcpy(sm + HAL_BOOT_SIG_SIZE, msg, len);

  int rc = crypto_sign_open(out, &mlen, sm,
                            (unsigned long long)(HAL_BOOT_SIG_SIZE + len), pk);
  return rc == 0 ? HAL_OK : HAL_ERR;
}


/* TweetNaCl's key generation and signing reference randombytes, and both halves
 * come in the same translation unit, so the symbol has to resolve even though a
 * bootloader calls neither.
 *
 * It traps rather than returning zeros. Zeros would make it look like it worked
 * and hand the caller an entirely predictable key; a hang on a path that should
 * be unreachable is the lesser outcome, and the watchdog ends it. Weak, so a
 * consumer that does want key generation can supply the real thing.
 */
__attribute__((weak)) void randombytes(unsigned char *p, unsigned long long n) {
  (void)p;
  (void)n;
  for (;;)
    ;
}
