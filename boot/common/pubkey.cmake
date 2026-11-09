# The public key a loader image trusts, turned into a C array at configure time.
#
# Shared by both stages because they verify with the same key: stage-1 checks
# stage-2, stage-2 checks the app, and a unit that trusted two different keys
# would have two different answers to "who may update this board".
#
# The key is a build input, never a file in the tree -- which key a unit trusts
# is a provisioning decision, and a committed one invites shipping it.
if(NOT NAVHAL_BOOT_PUBKEY)
  message(FATAL_ERROR
    "This image needs -DNAVHAL_BOOT_PUBKEY=<32-byte public key file>. Make one:\n"
    "  cmake -S tools/sign -B build-sign && cmake --build build-sign\n"
    "  ./build-sign/navhal_sign --genkey --key k.sec --pub k.pub")
endif()
if(NOT EXISTS "${NAVHAL_BOOT_PUBKEY}")
  message(FATAL_ERROR "NAVHAL_BOOT_PUBKEY does not exist: ${NAVHAL_BOOT_PUBKEY}")
endif()

file(SIZE "${NAVHAL_BOOT_PUBKEY}" _pk_size)
if(NOT _pk_size EQUAL 32)
  message(FATAL_ERROR
    "NAVHAL_BOOT_PUBKEY is ${_pk_size} bytes; an Ed25519 public key is 32. "
    "A 64-byte file is the SECRET key -- do not build that into an image.")
endif()

file(READ "${NAVHAL_BOOT_PUBKEY}" _pk_hex HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _pk_bytes "${_pk_hex}")
string(JOIN ", 0x" _pk_joined ${_pk_bytes})
set(BOOT_PUBKEY_INIT "0x${_pk_joined}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../common/boot_pubkey.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/boot_pubkey.h @ONLY)
