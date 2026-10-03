# Signing tool, built for the host rather than the target.
#
#   cmake -S tools/sign -B build-sign && cmake --build build-sign
#   ctest --test-dir build-sign          # the round-trip check
#
# Host build for the signing tool and its round-trip check.
#
#   cmake -S tools/sign -B build-sign && cmake --build build-sign
#   ./build-sign/sign_roundtrip        # signs, verifies, tampers, re-verifies
#
# Deliberately separate from the firmware build: this runs on the machine doing
# the signing, and it is the one place the private key appears.
cmake_minimum_required(VERSION 3.20)
project(navhal_sign C)
set(CMAKE_C_STANDARD 11)

set(NAVHAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CRYPTO ${NAVHAL_ROOT}/src/utils/crypto)

# The family guard in hal_bootmap.h keys off the generated config, which a host
# tool has no business configuring, so the one symbol it needs is given here.
add_compile_definitions(NAVHAL_CONFIG_FAMILY_STM32F4=1)
include_directories(
  ${NAVHAL_ROOT}/include
  ${CRYPTO}/monocypher
  ${CRYPTO}/sha256
)

set(CRYPTO_SRC
  ${CRYPTO}/monocypher/monocypher.c
  ${CRYPTO}/monocypher/monocypher-ed25519.c
  ${CRYPTO}/sha256/sha256.c
)

add_executable(navhal_sign navhal_sign.c ${CRYPTO_SRC})

# The check: the tool's output has to satisfy the target's verifier, and a single
# flipped byte has to break it. Linking boot_hash.c and the Monocypher backend is
# the point -- this is the firmware's verify path, not a reimplementation.
add_executable(sign_roundtrip
  sign_roundtrip.c
  ${CRYPTO}/boot_hash.c
  ${CRYPTO}/backend_monocypher.c
  ${CRYPTO_SRC}
)
enable_testing()
add_test(NAME sign_roundtrip COMMAND sign_roundtrip $<TARGET_FILE:navhal_sign>)
