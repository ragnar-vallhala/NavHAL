# NavHAL arch fragment — ARMv7E-M (Cortex-M4 / Cortex-M7).
#
# Sourced by the root CMakeLists.txt via `include(cmake/arch/${ARCH_ISA}.cmake)`.
# Sets the compiler/assembler/linker flag triple plus the per-arch test linker
# script reference. Adding a new ISA is one file under cmake/arch/ — the root
# CMakeLists.txt has no per-arch ladder.

# The armv7e-m ISA hosts both Cortex-M4 (single-precision FPU, fpv4-sp-d16)
# and Cortex-M7 (double-precision FPU, fpv5-d16). The FPU unit name is the
# only flag that differs between the two cores, so branch it on the resolved
# CMAKE_SYSTEM_PROCESSOR rather than forking the whole fragment.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m7")
    set(_ARCH_FPU "fpv5-d16")
else()
    set(_ARCH_FPU "fpv4-sp-d16")
endif()

if(USE_FPU)
    # _FPU_ENABLED + _DMA_ENABLED also leak into the C flags for legacy
    # consumers that #ifdef on them; the canonical capability source is now
    # navhal_target.h, but the -D path is kept until those last consumers move.
    set(FPU_FLAGS "-mfloat-abi=hard -mfpu=${_ARCH_FPU} -D_FPU_ENABLED -D_DMA_ENABLED")
    message(STATUS "FPU: Hardware enabled (${CMAKE_SYSTEM_PROCESSOR}, ${_ARCH_FPU})")
else()
    set(FPU_FLAGS "-mfloat-abi=soft")
    message(STATUS "FPU: Software emulation (${CMAKE_SYSTEM_PROCESSOR})")
endif()

set(ARCH_C_FLAGS    "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS} -O0 -g")
set(ARCH_ASM_FLAGS  "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS}")
set(ARCH_LINK_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib ${FPU_FLAGS}")

# Used by the `if(TEST)` block in the root CMakeLists.txt — each arch picks
# the linker setup that fits how its test ELF runs (custom linker + nostdlib
# for bare-metal Cortex; avr-libc-supplied startup for AVR).
set(NAVHAL_TEST_LINKER_FLAGS
    "-T ${CMAKE_CURRENT_SOURCE_DIR}/tests/arch/cortex-m4/linker.ld -nostdlib ${FPU_FLAGS}")
set(NAVHAL_TEST_EXTRA_FLAGS  "-march=armv7e-m")
set(NAVHAL_TEST_NEEDS_LIBGCC TRUE)
