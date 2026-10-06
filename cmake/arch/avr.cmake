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
# Link-time optimisation. The ATmega328P has 32 KiB of flash and a bootloader
# takes the top 2 KiB of it, so the ceiling for anything flashed through that
# bootloader is 30,720 bytes -- and the test image had grown to 32,382, which
# avrdude writes happily and then fails to verify at 0x7800 because the
# bootloader cannot overwrite itself. LTO takes the same image to 23,698 bytes,
# which is the difference between an on-target AVR suite that runs and one that
# cannot be flashed at all.
#
# No -Wl,-u,memset here, unlike the Cortex builds: avr-libc already supplies the
# compiler-called builtins, so forcing one in pulls libc's memset alongside the
# freestanding one this tree defines and the link fails on a duplicate symbol.
set(AVR_LTO_FLAGS "-flto")

set(ARCH_C_FLAGS    "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} ${AVR_LTO_FLAGS}")
set(ARCH_ASM_FLAGS  "-mmcu=${AVR_MCU}")
set(ARCH_LINK_FLAGS "-mmcu=${AVR_MCU} ${AVR_LTO_FLAGS}")

# Test ELF: avr-libc supplies crt0 + the default linker script, so no custom
# linker. `-mmcu=` already in ARCH_C_FLAGS picks the right device script
# (avr5.xn for atmega328p, etc.) — see docs/capabilities/atmega328p.md.
set(NAVHAL_TEST_LINKER_FLAGS "-mmcu=${AVR_MCU} ${AVR_LTO_FLAGS}")
set(NAVHAL_TEST_EXTRA_FLAGS  "")
set(NAVHAL_TEST_NEEDS_LIBGCC FALSE)
