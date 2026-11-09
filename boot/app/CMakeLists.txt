# A demo application, built from the root with -DBOOT_IMAGE=app.
#
# Needs signing before stage-2 will run it:
#   arm-none-eabi-objcopy -O binary build-app/boot/app/app body.bin
#   ./build-sign/navhal_sign --partition app --key k.sec --version 1 \
#        --in body.bin --out app.img
#   # flash app.img at 0x08020000 -- the header goes first
#
# No public key needed: an application verifies nothing, it is verified.
add_executable(app main.c)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)
