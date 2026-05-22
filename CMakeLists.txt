cmake_minimum_required(VERSION 3.20)

# Prevent macOS-specific flags
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_INIT "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "" CACHE STRING "" FORCE)

# Kconfig Integration
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KCONFIG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/kconfig.py")
set(KCONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Kconfig")
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.config")
set(GENERATED_CMAKE "${CMAKE_BINARY_DIR}/config.cmake")
set(GENERATED_HEADER "${CMAKE_BINARY_DIR}/autoconf.h")
set(GENERATED_NAVHAL_TARGET "${CMAKE_BINARY_DIR}/navhal_target.h")

message(STATUS "Running Kconfig configuration generation...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${KCONFIG_SCRIPT}
            --kconfig ${KCONFIG_FILE}
            --config ${CONFIG_FILE}
            --cmake-out ${GENERATED_CMAKE}
            --header-out ${GENERATED_HEADER}
            --navhal-target-out ${GENERATED_NAVHAL_TARGET}
            --sample-override "${SAMPLE}"
    RESULT_VARIABLE KCONFIG_RESULT
)
if(NOT KCONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Kconfig generation failed!")
endif()

# Include the generated CMake configuration which may provide TOOLCHAIN_PREFIX, BOARD, VENDOR, etc.
include(${GENERATED_CMAKE})

# Toolchain settings
# Allow configuration from KConfig (via GENERATED_CMAKE) to override the default.
if(NOT DEFINED TOOLCHAIN_PREFIX)
    set(TOOLCHAIN_PREFIX "arm-none-eabi-")
endif()
set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PREFIX}" CACHE STRING "Toolchain prefix for compiler binaries")
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_BINARY_SIZE ${TOOLCHAIN_PREFIX}size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# set(SYSROOT "/opt/homebrew/Cellar/arm-none-eabi-gcc/15.1.0/lib/gcc/arm-none-eabi/15.1.0")
# extern/NavHAL/CMakeLists.txt
#
# project() enables the C/ASM languages and must run on EVERY configure.
# Guarding it on CMAKE_PROJECT_NAME (a cache variable) made it run only on the
# first configure of a build directory — every later reconfigure skipped it,
# leaving the languages unenabled ("Cannot determine link language for
# target ..."). Detect the top-level build via a path comparison instead, which
# is re-evaluated fresh each configure and is true only for a standalone build.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(NavHAL C ASM)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json")


# Messages
message(STATUS "Building for board: ${BOARD}")
message(STATUS "Building for vendor: ${VENDOR}")
message(STATUS "Building for processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Paths — driven by the resolved Kconfig output (ARCH_ISA / VENDOR /
# FAMILY / BOARD / CMAKE_SYSTEM_PROCESSOR), not hardcoded slugs. M6's
# AVR port will populate src/arch/avr/, src/vendor/microchip/, etc.
# without further CMake edits.
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_BOARD ${CMAKE_CURRENT_SOURCE_DIR}/src/board/${BOARD})
set(SRC_ARCH ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH_ISA})
set(SRC_VENDOR ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/${VENDOR})
set(INCLUDE_PORT ${CMAKE_CURRENT_SOURCE_DIR}/include/port/${CMAKE_SYSTEM_PROCESSOR})
set(INCLUDE_FAMILY ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/${VENDOR}/family/${FAMILY}/include)
set(DIR_SAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/samples)
# ---- Architecture-specific compiler / linker flags ------------------------
# Cortex-M and AVR need entirely different flag sets: -mcpu/-mthumb + a custom
# linker script + -nostdlib for the bare-metal ARM build, versus -mmcu and the
# startup/linker that the avr-libc toolchain supplies itself. Everything below
# branches on the resolved CMAKE_SYSTEM_PROCESSOR.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m4")
    if(USE_FPU)
        set(FPU_FLAGS "-mfloat-abi=hard -mfpu=fpv4-sp-d16 -D_FPU_ENABLED -D_DMA_ENABLED")
        message(STATUS "FPU: Hardware enabled (Cortex-M4)")
    else()
        set(FPU_FLAGS "-mfloat-abi=soft")
        message(STATUS "FPU: Software emulation (Cortex-M4)")
    endif()
    set(ARCH_C_FLAGS    "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS} -O0 -g")
    set(ARCH_ASM_FLAGS  "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb ${FPU_FLAGS}")
    set(ARCH_LINK_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib ${FPU_FLAGS}")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
    # FAMILY is the avr-gcc -mmcu device name (atmega328p). F_CPU drives the
    # avr-libc timing routines; 16 MHz matches the ATmega328P reference board.
    set(FPU_FLAGS "")
    set(AVR_MCU "${FAMILY}")
    set(AVR_F_CPU "16000000UL")
    message(STATUS "AVR: -mmcu=${AVR_MCU}, F_CPU=${AVR_F_CPU}")
    set(ARCH_C_FLAGS    "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} -Os -g")
    set(ARCH_ASM_FLAGS  "-mmcu=${AVR_MCU}")
    set(ARCH_LINK_FLAGS "-mmcu=${AVR_MCU}")
else()
    set(FPU_FLAGS "")
    set(ARCH_C_FLAGS "")
    set(ARCH_ASM_FLAGS "")
    set(ARCH_LINK_FLAGS "")
    message(WARNING "No architecture flags predefined for core '${CMAKE_SYSTEM_PROCESSOR}'")
endif()

set(CMAKE_C_FLAGS "${ARCH_C_FLAGS} -Wall -std=c11")
set(CMAKE_ASM_FLAGS "${ARCH_ASM_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${ARCH_LINK_FLAGS}")
# C++ shares the arch flags; a C++ TU only appears in the C++ sample, which
# does enable_language(CXX). Bare-metal: no exceptions / RTTI / cxa-atexit.
set(CMAKE_CXX_FLAGS
    "${ARCH_C_FLAGS} -Wall -std=c++17 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics")
# subdirectories are commented, they will be added as code is added
if(NOT STANDALONE)
add_subdirectory(${SRC_DIR})
endif()

# TEST mode is exclusive: it builds its own `tests` executable from a GLOB of
# HAL sources (see the if(TEST) block below) and adds a `flash_tests` custom
# target. Pulling in samples/ here would also register each sample's own
# `flash` custom target (samples/*/CMakeLists.txt), conflicting with the test
# build. So when -DTEST=ON, skip samples/ and src/vendor/ entirely.
if(TEST)
  message(STATUS "TEST=ON — skipping samples/ and src/vendor/ (test build is self-contained)")
elseif(SAMPLE AND NOT STANDALONE)
  message(STATUS "Sample selected: ${SAMPLE} (with HAL)")
  message(STATUS  "Linker flag: ${CMAKE_EXE_LINKER_FLAGS}")
  # add_subdirectory(${SRC_BOARD})
  add_subdirectory(${SRC_VENDOR})
  # add_subdirectory(${SRC_VENDOR})
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples)
  # Sources
  set(SOURCES ${CORE_SOURCES} ${BOARD_SOURCES} ${VENDOR_SOURCES})
elseif(SAMPLE AND STANDALONE)
  message(STATUS "Sample selected: ${SAMPLE} (standalone)")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples)
else()
  message(STATUS "No SAMPLE defined. Building HAL only.")
  # add_subdirectory(${SRC_BOARD})
  add_subdirectory(${SRC_VENDOR})
  # add_subdirectory(${SRC_VENDOR})
endif()

add_custom_target(
  clean-all
  COMMAND ${CMAKE_COMMAND} --build . --target clean
  COMMAND ${CMAKE_COMMAND} -E rm -f ${PROJECT_NAME}.bin ${PROJECT_NAME}.elf
  COMMENT "Cleaning all build artifacts")

# Documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.out)

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
else()
  message(STATUS "Doxygen not found. 'doc' target will not be available.")
endif()

add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# Startup file (arch layer). The Cortex-M port ships an explicit startup.s;
# the AVR port relies on the avr-libc C runtime and has none.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
    set(STARTUP_FILE "")
else()
    set(STARTUP_FILE "${SRC_ARCH}/startup/startup.s")
endif()
# Export path to parent project if included via add_subdirectory()
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(STARTUP_FILE "${STARTUP_FILE}" PARENT_SCOPE)
endif()
# Tests
if(TEST)
  message("Tests enabled")
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
  )
  file(GLOB TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c)
  file(GLOB_RECURSE HAL_SRC
    ${SRC_VENDOR}/*.c
    ${SRC_ARCH}/*.c
  )
  message("Files: ${HAL_SRC}")
  add_compile_definitions(UNITY_OUTPUT_COLOR TEST)
  add_executable(tests ${TEST_SRC} ${STARTUP_FILE})
  target_link_libraries(tests PRIVATE c gcc)
  set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/tests/linker.ld -nostdlib ${FPU_FLAGS}")
  
  # Dynamic architecture options for test compiler flags
  set(TEST_FLAGS "-fno-exceptions" "-fno-unwind-tables" "-fno-asynchronous-unwind-tables")
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m4")
      list(APPEND TEST_FLAGS "-march=armv7e-m")
  endif()
  
  target_compile_options(tests PRIVATE ${TEST_FLAGS})
  target_sources(tests PRIVATE ${HAL_SRC})
  target_link_libraries(tests PRIVATE c gcc common)
  enable_testing()
  add_test(NAME tests COMMAND tests)
  
  # Bianry flasher
  if(NOT DEFINED FLASHER)
    set(FLASHER st-flash) # Default flasher for STM Boards
  endif()

  # Bianry flash address
  if(NOT DEFINED FLASH_ADDRESS)
   set(FLASH_ADDRESS 0x8000000) # Default address for stm32_nucleo_f401re
  endif()
   
   message(STATUS "Selected flasher: ${FLASHER}")
   message(STATUS "Selected flash address: ${FLASH_ADDRESS}")
   
  add_custom_target(flash_tests
     COMMAND ${CMAKE_OBJCOPY} -O binary tests tests.bin
     COMMAND ${FLASHER} --reset write tests.bin ${FLASH_ADDRESS}
     DEPENDS tests
     COMMENT "Converting ELF to BIN and flashing to board"
   )
   message(STATUS "TARGET File tests.bin")
endif()
