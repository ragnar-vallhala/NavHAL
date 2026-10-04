# Stage-1, built from the root with -DBOOT_IMAGE=stage1.
#
#   cmake -B build-stage1 -DBOARD=nucleo_f401re -DBOOT_IMAGE=stage1 \
#         -DNAVHAL_BOOT_PUBKEY=/path/to/key.pub \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake
#
# The public key is a build input, not a file in the tree: which key a unit trusts
# is a provisioning decision, and committing one would invite shipping it.
include(${CMAKE_CURRENT_SOURCE_DIR}/../common/pubkey.cmake)

add_executable(stage1 main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/updater.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_image.c)
target_include_directories(stage1 PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_SOURCE_DIR}/include)
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
