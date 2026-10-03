# Stage-1, built from the root with -DBOOT_IMAGE=stage1.
#
#   cmake -B build-stage1 -DBOARD=nucleo_f401re -DBOOT_IMAGE=stage1 \
#         -DNAVHAL_BOOT_PUBKEY=/path/to/key.pub \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake
#
# The public key is a build input, not a file in the tree: which key a unit trusts
# is a provisioning decision, and committing one would invite shipping it.
if(NOT NAVHAL_BOOT_PUBKEY)
  message(FATAL_ERROR
    "stage-1 needs -DNAVHAL_BOOT_PUBKEY=<32-byte public key file>. "
    "Make one with: cmake -S tools/sign -B build-sign && cmake --build build-sign "
    "&& ./build-sign/navhal_sign --genkey --key k.sec --pub k.pub")
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

# Turn the key into a C array at configure time.
file(READ "${NAVHAL_BOOT_PUBKEY}" _pk_hex HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _pk_bytes "${_pk_hex}")
string(JOIN ", 0x" _pk_joined ${_pk_bytes})
set(STAGE1_PUBKEY_INIT "0x${_pk_joined}")
configure_file(stage1_pubkey.h.in ${CMAKE_CURRENT_BINARY_DIR}/stage1_pubkey.h @ONLY)

add_executable(stage1 main.c recovery.c verify.c)
target_include_directories(stage1 PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(stage1 PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)

# Stage-1 owns two 16 KiB sectors and nothing may spill past them: the KV store
# starts where it ends. A build that overruns is a build failure, not something
# to discover when a save erases the loader.
add_custom_command(TARGET stage1 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "stage-1 size:"
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stage1>
)
