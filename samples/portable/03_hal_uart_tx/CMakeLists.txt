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

include_directories(${CMAKE_SOURCE_DIR}/include)

message(STATUS "Linker args ${CMAKE_EXE_LINKER_FLAGS}")

add_executable(${SAMPLE} main.c)

# target_link_libraries(${SAMPLE} hal)

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
# elf file size calculation
add_custom_command(
  TARGET ${SAMPLE}
  POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of elf file")
