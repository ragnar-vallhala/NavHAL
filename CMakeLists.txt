cmake_minimum_required(VERSION 3.20)
project(ARM_Cortex_HAL C ASM)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json")

# Toolchain settings
set(CMAKE_SYSTEM_NAME Generic)
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_BINARY_SIZE ${TOOLCHAIN_PREFIX}size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Defaults
set(CMAKE_SYSTEM_PROCESSOR
    cortex-m4
    CACHE STRING "Target processor" FORCE) # Processor on board
set(BOARD
    nucleo_f401re
    CACHE STRING "Target board")
set(VENDOR
    stm32
    CACHE STRING "Vendor")
set(STANDALONE
    OFF
    CACHE BOOL "Build sample without HAL")
set(SAMPLE
    ""
    CACHE STRING "Name of the sample to build")


# Messages
message(STATUS "Building for board: ${BOARD}")
message(STATUS "Building for vendor: ${VENDOR}")
message(STATUS "Building for processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Paths
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SRC_BOARD ${CMAKE_SOURCE_DIR}/src/board/${BOARD})
set(SRC_CORE ${CMAKE_SOURCE_DIR}/src/core)
set(SRC_VENDOR ${CMAKE_SOURCE_DIR}/src/vendor/${VENDOR})
set(DIR_SAMPLES ${CMAKE_SOURCE_DIR}/samples)

# Compiler Flags [UPDATE] thumb mode is on for now, can be removed later and
# choosen dynamically
set(CMAKE_C_FLAGS
    "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -Wall -O0 -g -std=c11") # C++
                                                                     # standard
                                                                     # 11
set(CMAKE_ASM_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb")

if(NOT STANDALONE)
  set(CMAKE_EXE_LINKER_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib") # can be
                                                                   # later
                                                                   # modified to
                                                                   # use
                                                                   # multiple
                                                                   # linker
                                                                   # scripts
                                                                   # dynamically
endif()
if(STANDALONE)
set(CMAKE_EXE_LINKER_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib") # can be
endif()
# subdirectories are commented, they will be added as code is added
if(NOT STANDALONE)
add_subdirectory(${SRC_DIR})
endif()
if(SAMPLE AND NOT STANDALONE)
  message(STATUS "Sample selected: ${SAMPLE} (with HAL)")
  message(STATUS  "Linker flag: ${CMAKE_EXE_LINKER_FLAGS}")
  # add_subdirectory(${SRC_BOARD})
  add_subdirectory(${SRC_CORE})
  # add_subdirectory(${SRC_VENDOR})
  add_subdirectory(${CMAKE_SOURCE_DIR}/samples)
  # Sources
  set(SOURCES ${CORE_SOURCES} ${BOARD_SOURCES} ${VENDOR_SOURCES})
elseif(SAMPLE AND STANDALONE)
  message(STATUS "Sample selected: ${SAMPLE} (standalone)")
  add_subdirectory(${CMAKE_SOURCE_DIR}/samples)
else()
  message(STATUS "No SAMPLE defined. Building HAL only.")
  # add_subdirectory(${SRC_BOARD})
  add_subdirectory(${SRC_CORE})
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

