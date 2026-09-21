# NavHAL CMake toolchain — bare-metal x86-64 (PC / QEMU).
#
# Use:  cmake -B <build-dir> \
#             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
#             -DSAMPLE=hal_x86_hello
#
# Freestanding 64-bit kernel built with the native host gcc (no cross prefix):
# the ELF64 boots via multiboot2 (GRUB) and runs under qemu-system-x86_64.
# cmake/arch/x86_64.cmake supplies the freestanding flags; the run target
# (samples/x86/*/CMakeLists.txt -> tools/qemu/run.sh) wraps the ELF in a GRUB
# rescue ISO.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)
set(CMAKE_OBJCOPY      objcopy)
set(CMAKE_SIZE         size)
# Project-specific name used by samples/*/CMakeLists.txt; mirrors CMAKE_SIZE.
set(CMAKE_BINARY_SIZE  size)

# Skip the implicit-link compiler check that fails on a freestanding target.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(NAVHAL_DEFCONFIG "${CMAKE_CURRENT_LIST_DIR}/../defconfigs/x86_64_pc_qemu.defconfig"
    CACHE FILEPATH "Kconfig fragment seeded into .config before generation")
