# C++ blink sample. Mirrors samples/01_hal_blink/CMakeLists.txt but
# enables the CXX language (root project() declares only C and ASM) and
# compiles main.cpp instead of main.c.
#
# Bare-metal C++ flags follow the standard -nostdlib drill:
#   -fno-exceptions / -fno-rtti     : no C++ runtime metadata
#   -fno-use-cxa-atexit             : no __cxa_atexit (newlib stub missing)
#   -fno-threadsafe-statics         : no guard variables on local statics
# CMAKE_CXX_COMPILER is set in the root CMakeLists alongside CC/AS so the
# arm-none-eabi-g++ driver is picked up here automatically.
enable_language(CXX)
# CMAKE_CXX_FLAGS is set arch-conditionally by the root CMakeLists, alongside
# CMAKE_C_FLAGS — no per-sample compiler flags needed here.

if(NOT DEFINED FLASHER)
  set(FLASHER st-flash)
endif()
if(NOT DEFINED FLASH_ADDRESS)
  set(FLASH_ADDRESS 0x8000000)
endif()

message(STATUS "Selected flasher: ${FLASHER}")
message(STATUS "Selected flash address: ${FLASH_ADDRESS}")

include_directories(${CMAKE_SOURCE_DIR}/include)

message(STATUS "Linker args ${CMAKE_EXE_LINKER_FLAGS}")

add_executable(${SAMPLE} main.cpp)

target_link_libraries(${SAMPLE} PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)

if(NOT DEFINED FLASHER OR NOT DEFINED FLASH_ADDRESS)
  message(
    FATAL_ERROR
      "FLASHER and FLASH_ADDRESS must be defined to use 'flash' target.")
endif()

# Flash target — shared, arch-aware (st-flash / avrdude); see samples/flash.cmake.
include(${CMAKE_CURRENT_LIST_DIR}/../../flash.cmake)
message(STATUS "TARGET File ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin")

add_custom_command(
  TARGET ${SAMPLE}
  POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of elf file")
