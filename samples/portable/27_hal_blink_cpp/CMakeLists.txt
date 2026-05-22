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

set(CMAKE_CXX_FLAGS
  "-mcpu=${CMAKE_SYSTEM_PROCESSOR} ${ARCH_FLAGS} -Wall -O0 -g -std=c++17 ${FPU_FLAGS} -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics")

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

add_custom_target(flash
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin
  COMMAND ${FLASHER} --reset write ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin ${FLASH_ADDRESS}
  DEPENDS ${SAMPLE}
  COMMENT "Converting ELF to BIN and flashing to board"
)
message(STATUS "TARGET File ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin")

add_custom_command(
  TARGET ${SAMPLE}
  POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of elf file")
