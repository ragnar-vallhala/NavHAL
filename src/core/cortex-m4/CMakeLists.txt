# Use target-based include to ensure NavHAL headers are prioritized even when built as submodule

set(HAL_SOURCES)

if(CONFIG_DRV_GPIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c)
endif()
if(CONFIG_DRV_CLOCK)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c)
endif()
if(CONFIG_DRV_UART)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c)
endif()
if(CONFIG_DRV_TIMER)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer.c)
endif()

# Always include startup assembly code
list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.s)

if(CONFIG_DRV_INTERRUPT)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/interrupt/interrupt.c)
endif()
if(CONFIG_DRV_PWM)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pwm/pwm.c)
endif()
if(CONFIG_DRV_I2C)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/i2c/i2c.c)
endif()
if(CONFIG_DRV_SPI)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/spi/spi.c)
endif()
if(CONFIG_DRV_FLASH)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/flash/flash.c)
endif()
if(CONFIG_DRV_DMA)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dma/dma.c)
endif()
if(CONFIG_DRV_FPU)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/fpu/fpu.c)
endif()
if(CONFIG_DRV_CRC)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crc/crc.c)
endif()
if(CONFIG_DRV_SDIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdio/sdio.c)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdio/diskio.c)
endif()
if(CONFIG_DRV_DWT)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dwt/dwt.c)
endif()

add_library(hal STATIC
    ${HAL_SOURCES}
    $<TARGET_OBJECTS:common>
)

target_link_libraries(hal gcc)
target_include_directories(hal PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_BINARY_DIR}
)

if(CONFIG_DRV_DMA)
    target_compile_definitions(hal PUBLIC _DMA_ENABLED)
endif()

