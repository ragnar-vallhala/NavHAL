# NavHAL CMake toolchain — AVR / ATmega328P.
#
# Use:  cmake -B <build-dir> \
#             -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-toolchain.cmake \
#             -DSAMPLE=hal_blink
#
# Sets the cross-compiler and points the project at the matching defconfig
# (cmake/defconfigs/avr_atmega328p.defconfig). When the top-level
# CMakeLists.txt sees NAVHAL_DEFCONFIG, it copies the fragment into
# .config before tools/kconfig.py runs, so the resulting build picks up
# CONFIG_ARCH_AVR8 / VENDOR_MICROCHIP / FAMILY_ATMEGA328P / BOARD_ATMEGA328P
# without any ad-hoc .config patching.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_COMPILER   avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)
set(CMAKE_OBJCOPY      avr-objcopy)
set(CMAKE_SIZE         avr-size)
# Project-specific name used by samples/*/CMakeLists.txt; mirrors CMAKE_SIZE
# until those are migrated to the conventional name.
set(CMAKE_BINARY_SIZE  avr-size)

# Bare-metal: skip the implicit-link compiler check that fails without
# a hosted libc.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Hint to the top-level CMakeLists.txt which Kconfig fragment to seed.
set(NAVHAL_DEFCONFIG "${CMAKE_CURRENT_LIST_DIR}/../defconfigs/avr_atmega328p.defconfig"
    CACHE FILEPATH "Kconfig fragment seeded into .config before generation" FORCE)
