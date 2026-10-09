# NavHAL arch fragment — AVR8 (ATmega328P et al.).
#
# Sourced by the root CMakeLists.txt via `include(cmake/arch/${ARCH_ISA}.cmake)`.
# AVR has no hardware FPU and no DMA on this family, so FPU_FLAGS stays empty.
#
# F_CPU drives every timing path on this arch -- UART baud, the timebase, PWM,
# the I2C SCL divisor and the timer -- and it is the board's crystal, not a chip
# fact, so it comes from the board's Kconfig. No default here: a board that
# describes no crystal must say so rather than silently inherit 16 MHz.

set(FPU_FLAGS "")
set(AVR_MCU   "${FAMILY}")
if(NOT DEFINED CONFIG_NUM_BOARD_XTAL_FREQ_HZ OR CONFIG_NUM_BOARD_XTAL_FREQ_HZ STREQUAL "")
  message(FATAL_ERROR
    "AVR board must describe NUM_BOARD_XTAL_FREQ_HZ in its Kconfig; "
    "F_CPU is derived from it.")
endif()
set(AVR_F_CPU "${CONFIG_NUM_BOARD_XTAL_FREQ_HZ}UL")
message(STATUS "AVR: -mmcu=${AVR_MCU}, F_CPU=${AVR_F_CPU}")

# Optimisation level comes from the build profile, not from here: a level
# hardcoded in ARCH_C_FLAGS is silently overridden by the profile flags
# that CMake appends after it.
# Flash is 32 KiB and a bootloader owns the top 2 KiB of it, so anything flashed
# through that bootloader has 30,720 bytes to live in. Two mechanisms earn their
# place here, and they are complementary rather than alternatives:
#
# Section GC drops what is never reached; avr-libc's startup and vectors live in
# sections the linker keeps regardless.
#
# LTO is what made the on-target suite flashable at all. The test image had grown
# to 32,382 bytes, which avrdude writes happily and then fails to verify at
# 0x7800, because the bootloader cannot overwrite itself and every byte past that
# address is quietly dropped. The error reads "verification mismatch", which looks
# like a bad board rather than an image 1,662 bytes too large.
#
# No -Wl,-u,memset here, unlike the Cortex builds: avr-libc already supplies the
# compiler-called builtins, so forcing one in pulls libc's memset alongside the
# freestanding one this tree defines, and the link fails on a duplicate symbol.
set(AVR_LTO_FLAGS "-flto")

set(ARCH_C_FLAGS    "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} ${AVR_LTO_FLAGS} -ffunction-sections -fdata-sections")
set(ARCH_ASM_FLAGS  "-mmcu=${AVR_MCU}")
set(ARCH_LINK_FLAGS "-mmcu=${AVR_MCU} ${AVR_LTO_FLAGS} -Wl,--gc-sections")
set(NAVHAL_TEST_EXTRA_FLAGS  "")
set(NAVHAL_TEST_NEEDS_LIBGCC FALSE)
