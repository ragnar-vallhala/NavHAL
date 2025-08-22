cmake_minimum_required(VERSION 3.20)

# Prevent macOS-specific flags
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_INIT "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "" CACHE STRING "" FORCE)

# Toolchain settings
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_BINARY_SIZE ${TOOLCHAIN_PREFIX}size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# set(SYSROOT "/opt/homebrew/Cellar/arm-none-eabi-gcc/15.1.0/lib/gcc/arm-none-eabi/15.1.0")
# extern/NavHAL/CMakeLists.txt
if(NOT DEFINED CMAKE_PROJECT_NAME)
    project(NavHAL C ASM)
else()
  add_compile_definitions(SUBMODULE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json")



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
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_BOARD ${CMAKE_CURRENT_SOURCE_DIR}/src/board/${BOARD})
set(SRC_CORE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
set(SRC_VENDOR ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/${VENDOR})
set(DIR_SAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/samples)
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
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples)
  # Sources
  set(SOURCES ${CORE_SOURCES} ${BOARD_SOURCES} ${VENDOR_SOURCES})
elseif(SAMPLE AND STANDALONE)
  message(STATUS "Sample selected: ${SAMPLE} (standalone)")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples)
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

set(STARTUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/${CMAKE_SYSTEM_PROCESSOR}/startup/startup.s PARENT_SCOPE)
# Tests
if(TEST)
  message("Tests enabled")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern/unity/src ${CMAKE_CURRENT_SOURCE_DIR}/include)
  file(GLOB TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c)
  file(GLOB UNITY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/extern/unity/src/*.c)
  file(GLOB_RECURSE HAL_SRC ${SRC_CORE}/*.c )
  message("Files: ${HAL_SRC}")
  add_compile_definitions(UNITY_OUTPUT_COLOR TEST)
  add_executable(tests ${TEST_SRC} ${UNITY_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/src/core/cortex-m4/startup/startup.s)
  target_link_libraries(tests PRIVATE c gcc)
  set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/tests/linker.ld -nostdlib")
  target_compile_options(tests PRIVATE -march=armv7e-m -fno-exceptions 
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  )
  target_sources(tests PRIVATE ${HAL_SRC})
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
   
   add_custom_target(hal_test
     COMMAND ${CMAKE_OBJCOPY} -O binary tests tests.bin
     COMMAND ${FLASHER} write tests.bin ${FLASH_ADDRESS}
     DEPENDS tests
     COMMENT "Converting ELF to BIN and flashing to board"
   )
   message(STATUS "TARGET File tests.bin")
endif()
