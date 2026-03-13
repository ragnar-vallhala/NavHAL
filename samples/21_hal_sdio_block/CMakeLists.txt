cmake_minimum_required(VERSION 3.10)

set(EXECUTABLE_NAME "hal_sdio_block")

# Define source files for this sample
set(SAMPLE_SOURCES
    "main.c"
)

# Add executable
add_executable(${EXECUTABLE_NAME} ${SAMPLE_SOURCES})

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)

# Include directories
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)



# Generate .bin file using arm-none-eabi-objcopy
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.bin
    COMMENT "Converting ELF to BIN"
)

# Custom target for flashing
add_custom_target(flash
    COMMAND st-flash --connect-under-reset write ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.bin 0x08000000
    COMMAND st-flash reset
    DEPENDS ${EXECUTABLE_NAME}
    COMMENT "Flashing ${EXECUTABLE_NAME}.bin to board and resetting"
)
