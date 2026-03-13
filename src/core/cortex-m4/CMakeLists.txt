# Use target-based include to ensure NavHAL headers are prioritized even when built as submodule

add_library(hal 
STATIC
${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c 
${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c
${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c
${CMAKE_CURRENT_SOURCE_DIR}/timer/timer.c
${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.s
${CMAKE_CURRENT_SOURCE_DIR}/interrupt/interrupt.c
${CMAKE_CURRENT_SOURCE_DIR}/pwm/pwm.c
${CMAKE_CURRENT_SOURCE_DIR}/i2c/i2c.c
${CMAKE_CURRENT_SOURCE_DIR}/flash/flash.c
${CMAKE_CURRENT_SOURCE_DIR}/dma/dma.c
${CMAKE_CURRENT_SOURCE_DIR}/fpu/fpu.c
${CMAKE_CURRENT_SOURCE_DIR}/crc/crc.c
${CMAKE_CURRENT_SOURCE_DIR}/sdio/sdio.c
${CMAKE_CURRENT_SOURCE_DIR}/sdio/diskio.c
)

target_link_libraries(hal gcc common)
target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_compile_definitions(hal PUBLIC _DMA_ENABLED)
