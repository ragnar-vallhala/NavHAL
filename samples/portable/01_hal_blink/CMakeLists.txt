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


# Flashing board — the toolchain differs per target: st-flash writes a raw
# binary at a flash address; avrdude writes an Intel-HEX image through a
# programmer. AVR_PROGRAMMER / AVR_PORT can be overridden at configure time.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
  if(NOT DEFINED AVR_PROGRAMMER)
    set(AVR_PROGRAMMER arduino)
  endif()
  if(NOT DEFINED AVR_PORT)
    set(AVR_PORT /dev/ttyACM0)
  endif()
  add_custom_target(flash
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.hex
    COMMAND ${FLASHER} -p atmega328p -c ${AVR_PROGRAMMER} -P ${AVR_PORT} -U flash:w:${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.hex:i
    DEPENDS ${SAMPLE}
    COMMENT "Converting ELF to Intel HEX and flashing via avrdude"
  )
else()
  add_custom_target(flash
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin
    COMMAND ${FLASHER} --reset write ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin ${FLASH_ADDRESS}
    DEPENDS ${SAMPLE}
    COMMENT "Converting ELF to BIN and flashing to board"
  )
endif()
message(STATUS "TARGET File ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin")
# elf file size calculation
add_custom_command(
  TARGET ${SAMPLE}
  POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of elf file")
