# NavHAL CMake toolchain — ARM Cortex-M7 / arm-none-eabi (Nucleo-F767ZI).
#
# Use:  cmake -B <build-dir> \
#             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake \
#             -DSAMPLE=01_hal_blink
#
# Mirrors arm-none-eabi-toolchain.cmake (Cortex-M4) but sets
# CMAKE_SYSTEM_PROCESSOR=cortex-m7 and seeds the F767 defconfig. The same
# arm-none-eabi binaries serve both cores; cmake/arch/armv7e-m.cmake picks the
# right -mcpu / -mfpu from CMAKE_SYSTEM_PROCESSOR.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m7)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy)
set(CMAKE_SIZE         arm-none-eabi-size)
# Project-specific name used by samples/*/CMakeLists.txt; mirrors CMAKE_SIZE
# until those are migrated to the conventional name.
set(CMAKE_BINARY_SIZE  arm-none-eabi-size)

# Skip compiler checks that fail on bare-metal.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Default Kconfig fragment for this target.
set(NAVHAL_DEFCONFIG "${CMAKE_CURRENT_LIST_DIR}/../defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig"
    CACHE FILEPATH "Kconfig fragment seeded into .config before generation")
