cmake_minimum_required(VERSION 3.20)

# Prevent macOS-specific flags
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_INIT "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "" CACHE STRING "" FORCE)

# Load the toolchain file early. CMake normally loads it at project(),
# which is well below the Kconfig invocation, but we need its variables
# (notably NAVHAL_DEFCONFIG) visible to the seeding logic below. CMake
# will load it again at project(); toolchain files are idempotent so the
# re-load is harmless.
if(CMAKE_TOOLCHAIN_FILE)
    include("${CMAKE_TOOLCHAIN_FILE}")
endif()

# Kconfig Integration
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KCONFIG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/kconfig.py")
set(KCONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Kconfig")
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.config")
set(GENERATED_CMAKE "${CMAKE_BINARY_DIR}/config.cmake")
set(GENERATED_HEADER "${CMAKE_BINARY_DIR}/autoconf.h")
set(GENERATED_NAVHAL_TARGET "${CMAKE_BINARY_DIR}/navhal_target.h")

# Defconfig seeding. A toolchain file (cmake/toolchains/*.cmake) can set
# NAVHAL_DEFCONFIG to a Kconfig fragment naming the target arch / vendor /
# family / board. When .config doesn't already exist, copy the fragment in
# so the kconfig.py run below resolves the right target. This replaces
# ad-hoc .config patching in cross-target build scripts.
if(DEFINED NAVHAL_DEFCONFIG AND NOT EXISTS "${CONFIG_FILE}")
    if(NOT EXISTS "${NAVHAL_DEFCONFIG}")
        message(FATAL_ERROR "NAVHAL_DEFCONFIG points to '${NAVHAL_DEFCONFIG}' which does not exist")
    endif()
    message(STATUS "Seeding .config from ${NAVHAL_DEFCONFIG}")
    configure_file("${NAVHAL_DEFCONFIG}" "${CONFIG_FILE}" COPYONLY)
endif()

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

# Make the Kconfig-generated navhal_target.h reachable to every translation
# unit so port headers can bridge NAVHAL_HAS_* into the legacy _*_ENABLED
# flags. Without this the cap macros silently fall back to the port header's
# hardcoded defaults.
include_directories(${CMAKE_BINARY_DIR})

# Toolchain settings.
# Preferred path: caller passes -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/<arch>.cmake
# and the toolchain file sets the compiler. Legacy path: no toolchain file,
# derive the compiler from CONFIG_TOOLCHAIN_PREFIX in .config. This keeps
# `cmake -B build -DSAMPLE=hal_blink` working without a toolchain file for
# Cortex-M4 / arm-none-eabi.
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(NOT DEFINED TOOLCHAIN_PREFIX)
        set(TOOLCHAIN_PREFIX "arm-none-eabi-")
    endif()
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PREFIX}" CACHE STRING "Toolchain prefix for compiler binaries")
    set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
    set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
    set(CMAKE_BINARY_SIZE  ${TOOLCHAIN_PREFIX}size)
endif()
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# set(SYSROOT "/opt/homebrew/Cellar/arm-none-eabi-gcc/15.1.0/lib/gcc/arm-none-eabi/15.1.0")
# extern/NavHAL/CMakeLists.txt
#
# ---- Release version --------------------------------------------------------
# include/navhal.h is the single source of truth for the SemVer release version
# (it is the header applications include). We mirror it here so the build and
# tooling can use ${PROJECT_VERSION}, and we FATAL_ERROR if the two ever
# disagree: bumping one without the other is a release-process bug, not
# something we want to silently build. This check runs on every configure,
# including when NavHAL is consumed as a submodule.
set(NAVHAL_VERSION 0.2.3) # keep in sync with VERSION_MAJOR/MINOR/PATCH in include/navhal.h
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/navhal.h" _navhal_h)
string(REGEX MATCH "#define[ \t]+VERSION_MAJOR[ \t]+([0-9]+)" _ "${_navhal_h}")
set(_navhal_h_major "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define[ \t]+VERSION_MINOR[ \t]+([0-9]+)" _ "${_navhal_h}")
set(_navhal_h_minor "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define[ \t]+VERSION_PATCH[ \t]+([0-9]+)" _ "${_navhal_h}")
set(_navhal_h_patch "${CMAKE_MATCH_1}")
set(_navhal_h_version "${_navhal_h_major}.${_navhal_h_minor}.${_navhal_h_patch}")
if(NOT NAVHAL_VERSION STREQUAL _navhal_h_version)
    message(FATAL_ERROR
        "NavHAL version mismatch: CMakeLists NAVHAL_VERSION='${NAVHAL_VERSION}' but "
        "include/navhal.h declares '${_navhal_h_version}'. navhal.h is the source of "
        "truth — bump VERSION_MAJOR/MINOR/PATCH and NAVHAL_VERSION together.")
endif()
#
# project() enables the C/ASM languages and must run on EVERY configure.
# Guarding it on CMAKE_PROJECT_NAME (a cache variable) made it run only on the
# first configure of a build directory — every later reconfigure skipped it,
# leaving the languages unenabled ("Cannot determine link language for
# target ..."). Detect the top-level build via a path comparison instead, which
# is re-evaluated fresh each configure and is true only for a standalone build.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(NavHAL VERSION ${NAVHAL_VERSION} LANGUAGES C ASM)
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
# Each supported ISA owns a fragment under cmake/arch/<ARCH_ISA>.cmake that
# fills in ARCH_C_FLAGS / ARCH_ASM_FLAGS / ARCH_LINK_FLAGS / FPU_FLAGS plus
# the NAVHAL_TEST_* slots the if(TEST) block consumes. Adding a new ISA is
# one file there — this root CMakeLists.txt has no per-arch ladder.
set(_NAVHAL_ARCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arch/${ARCH_ISA}.cmake")
if(NOT EXISTS "${_NAVHAL_ARCH_FILE}")
    message(FATAL_ERROR
        "No CMake arch fragment for ARCH_ISA='${ARCH_ISA}'. "
        "Expected: ${_NAVHAL_ARCH_FILE}")
endif()
include("${_NAVHAL_ARCH_FILE}")

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
  # tests/ is added so #include "portable/test_x.h" / "cap/x/test_x.h" /
  # "arch/<arch>/test_x.h" resolves naturally from main.c.
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
  )

  # Test sources split into tiers (see tests/main.c for the rationale):
  #   tests/*.c               — framework files (main, navtest_state, stubs)
  #   tests/portable/*.c      — HAL-only black-box tests; every arch builds these
  #   tests/cap/*/*.c         — capability-gated; each file is wrapped in #if
  #                             NAVHAL_HAS_<cap>, so it self-omits when the cap
  #                             is off — globbing them all is safe.
  #   tests/arch/<arch>/*.c   — white-box per-arch tests; only the matching
  #                             arch's directory is compiled in.
  file(GLOB          TEST_FRAMEWORK_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c)
  file(GLOB_RECURSE  TEST_PORTABLE_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/tests/portable/*.c)
  file(GLOB_RECURSE  TEST_CAP_SRC       ${CMAKE_CURRENT_SOURCE_DIR}/tests/cap/*.c)
  file(GLOB          TEST_ARCH_SRC      ${CMAKE_CURRENT_SOURCE_DIR}/tests/arch/${CMAKE_SYSTEM_PROCESSOR}/*.c)
  set(TEST_SRC ${TEST_FRAMEWORK_SRC} ${TEST_PORTABLE_SRC} ${TEST_CAP_SRC} ${TEST_ARCH_SRC})

  file(GLOB_RECURSE HAL_SRC
    ${SRC_VENDOR}/*.c
    ${SRC_ARCH}/*.c
  )
  message("Files: ${HAL_SRC}")
  add_compile_definitions(UNITY_OUTPUT_COLOR TEST)
  add_executable(tests ${TEST_SRC} ${STARTUP_FILE})

  # Per-arch test linker + flags. NAVHAL_TEST_LINKER_FLAGS,
  # NAVHAL_TEST_EXTRA_FLAGS, NAVHAL_TEST_NEEDS_LIBGCC all come from the
  # cmake/arch/<ARCH_ISA>.cmake fragment included above. No arch ladder here.
  set(TEST_FLAGS "-fno-exceptions" "-fno-unwind-tables" "-fno-asynchronous-unwind-tables")
  if(NAVHAL_TEST_EXTRA_FLAGS)
      list(APPEND TEST_FLAGS ${NAVHAL_TEST_EXTRA_FLAGS})
  endif()
  set(CMAKE_EXE_LINKER_FLAGS "${NAVHAL_TEST_LINKER_FLAGS}")
  if(NAVHAL_TEST_NEEDS_LIBGCC)
      target_link_libraries(tests PRIVATE c gcc)
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
