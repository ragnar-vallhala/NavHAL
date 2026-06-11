# NavHAL arch fragment — AVR8 (ATmega328P et al.).
#
# Sourced by the root CMakeLists.txt via `include(cmake/arch/${ARCH_ISA}.cmake)`.
# AVR has no hardware FPU and no DMA on this family, so FPU_FLAGS stays empty;
# F_CPU drives the avr-libc timing routines (16 MHz matches the ATmega328P
# reference board).

set(FPU_FLAGS "")
set(AVR_MCU   "${FAMILY}")
set(AVR_F_CPU "16000000UL")
message(STATUS "AVR: -mmcu=${AVR_MCU}, F_CPU=${AVR_F_CPU}")

set(ARCH_C_FLAGS    "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} -Os -g")
set(ARCH_ASM_FLAGS  "-mmcu=${AVR_MCU}")
set(ARCH_LINK_FLAGS "-mmcu=${AVR_MCU}")

# Test ELF: avr-libc supplies crt0 + the default linker script, so no custom
# linker. `-mmcu=` already in ARCH_C_FLAGS picks the right device script
# (avr5.xn for atmega328p, etc.) — see docs/capabilities/atmega328p.md.
set(NAVHAL_TEST_LINKER_FLAGS "-mmcu=${AVR_MCU}")
set(NAVHAL_TEST_EXTRA_FLAGS  "")
set(NAVHAL_TEST_NEEDS_LIBGCC FALSE)
