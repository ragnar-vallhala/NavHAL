# Shared, arch-aware `flash` target for samples.
#
# The flash toolchain differs per target: st-flash writes a raw binary at a
# flash address (Cortex-M), while avrdude writes an Intel-HEX image through a
# programmer (AVR). A sample's CMakeLists includes this file instead of
# duplicating the target; it expects ${SAMPLE}, ${FLASHER} and
# ${CMAKE_OBJCOPY} to already be set.
#
# AVR_PROGRAMMER / AVR_PORT / FLASH_ADDRESS may be overridden at configure
# time (e.g. -DAVR_PORT=/dev/ttyUSB1).

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
  if(NOT DEFINED AVR_PROGRAMMER)
    set(AVR_PROGRAMMER arduino)
  endif()
  if(NOT DEFINED AVR_PORT)
    set(AVR_PORT /dev/ttyUSB0)
  endif()
  add_custom_target(flash
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.hex
    COMMAND ${FLASHER} -p atmega328p -c ${AVR_PROGRAMMER} -P ${AVR_PORT} -U flash:w:${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.hex:i
    DEPENDS ${SAMPLE}
    COMMENT "Converting ELF to Intel HEX and flashing via avrdude")
else()
  if(NOT DEFINED FLASH_ADDRESS)
    set(FLASH_ADDRESS 0x8000000)
  endif()
  add_custom_target(flash
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${SAMPLE}> ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin
    COMMAND ${FLASHER} --reset write ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE}.bin ${FLASH_ADDRESS}
    DEPENDS ${SAMPLE}
    COMMENT "Converting ELF to BIN and flashing to board")
endif()
