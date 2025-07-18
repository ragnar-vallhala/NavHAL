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

add_executable(${SAMPLE} main.c startup.s)
if(DEFINED STANDALONE)
  set(CMAKE_EXE_LINKER_FLAGS
      "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib")
endif()


if(NOT DEFINED FLASHER OR NOT DEFINED FLASH_ADDRESS)
  message(
    FATAL_ERROR
      "FLASHER and FLASH_ADDRESS must be defined to use 'flash' target.")
endif()


# Flashing board
add_custom_target(flash
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin
  COMMAND ${FLASHER} write ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin ${FLASH_ADDRESS}
  DEPENDS ${SAMPLE}
  COMMENT "Converting ELF to BIN and flashing to board"
)

# elf file size calculation
add_custom_command(
  TARGET ${SAMPLE}
  POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of elf file")
