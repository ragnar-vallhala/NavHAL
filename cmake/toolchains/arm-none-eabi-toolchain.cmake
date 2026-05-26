# NavHAL CMake toolchain — ARM Cortex-M4 / arm-none-eabi (e.g. Nucleo-F401RE).
#
# Use:  cmake -B <build-dir> \
#             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake \
#             -DSAMPLE=hal_blink
#
# Both the cap-contract and sample-matrix tooling default to this configuration
# when no toolchain file is passed (CMakeLists.txt has a legacy
# CONFIG_TOOLCHAIN_PREFIX-based code path), so passing the toolchain file
# explicitly is optional. It becomes useful when you want the project to
# behave the same way as the AVR build (toolchain-file driven), or when you
# rely on NAVHAL_DEFCONFIG to seed a fresh .config from a checked-in fragment.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)

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
set(NAVHAL_DEFCONFIG "${CMAKE_CURRENT_LIST_DIR}/../defconfigs/cortex-m4_stm32f4_nucleo_f401re.defconfig"
    CACHE FILEPATH "Kconfig fragment seeded into .config before generation")
