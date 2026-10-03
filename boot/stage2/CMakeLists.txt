# Stage-2, built from the root with -DBOOT_IMAGE=stage2.
#
# The output needs signing before a board will run it:
#   arm-none-eabi-objcopy -O binary build-stage2/boot/stage2/stage2 body.bin
#   ./build-sign/navhal_sign --partition stage2 --key k.sec --version 1 \
#        --in body.bin --out stage2.img
#   # flash stage2.img at 0x08010000 -- the header goes first
add_executable(stage2 main.c)
target_include_directories(stage2 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(stage2 PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)
