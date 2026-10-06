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
# Section GC matters most here: 32 KiB of flash, and the on-target test image
# has already had to be squeezed once (that is why Debug builds -Os on this
# arch). avr-libc's own startup and vectors are in sections the linker keeps.
set(ARCH_C_FLAGS    "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} -ffunction-sections -fdata-sections")
set(ARCH_ASM_FLAGS  "-mmcu=${AVR_MCU}")
set(ARCH_LINK_FLAGS "-mmcu=${AVR_MCU} -Wl,--gc-sections")

# Test ELF: avr-libc supplies crt0 + the default linker script, so no custom
# linker. `-mmcu=` already in ARCH_C_FLAGS picks the right device script
# (avr5.xn for atmega328p, etc.) — see docs/capabilities/atmega328p.md.
set(NAVHAL_TEST_LINKER_FLAGS "-mmcu=${AVR_MCU} -Wl,--gc-sections")
set(NAVHAL_TEST_EXTRA_FLAGS  "")
set(NAVHAL_TEST_NEEDS_LIBGCC FALSE)
