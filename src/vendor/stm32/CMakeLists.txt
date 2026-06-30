# STM32 vendor IP drivers (gpio, uart, timer, …). Arch sources (interrupt/
# NVIC, fpu, dwt, startup.s, SysTick-backed timebase) live under
# src/arch/armv7e-m/ and are pulled in via the SRC_ARCH path that the root
# CMakeLists.txt sets. Everything is aggregated into one `hal` library
# target until M4 reworks the build into per-layer libraries.

set(HAL_SOURCES)

if(CONFIG_DRV_GPIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c)
endif()
if(CONFIG_DRV_CLOCK)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c)
endif()
if(CONFIG_DRV_UART)
    # The STM32F7 USART IP (ISR/RDR/TDR) differs from the F4 (SR/DR), so the F7
    # port ships its own driver file. Selecting by family here keeps the frozen
    # F4 uart.c untouched. (To be unified under the vendor-backend vtable in M9.)
    if(CONFIG_FAMILY_STM32F7)
        list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart_f7.c)
    else()
        list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c)
    endif()
endif()
if(CONFIG_DRV_TIMER)
    list(APPEND HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer.c
        ${SRC_ARCH}/timebase/timebase.c
    )
endif()

# Startup is arch-layer code; always linked in.
list(APPEND HAL_SOURCES ${SRC_ARCH}/startup/startup.s)

if(CONFIG_DRV_INTERRUPT)
    list(APPEND HAL_SOURCES ${SRC_ARCH}/interrupt/interrupt.c)
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
    list(APPEND HAL_SOURCES ${SRC_ARCH}/fpu/fpu.c)
endif()
if(CONFIG_DRV_CRC)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crc/crc.c)
endif()
if(CONFIG_DRV_SDIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdio/sdio.c)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdio/diskio.c)
endif()
if(CONFIG_DRV_DWT)
    list(APPEND HAL_SOURCES ${SRC_ARCH}/dwt/dwt.c)
endif()

add_library(hal STATIC
    ${HAL_SOURCES}
    $<TARGET_OBJECTS:common>
)

target_link_libraries(hal gcc)
target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
    ${CMAKE_BINARY_DIR}
)

if(CONFIG_DRV_DMA)
    target_compile_definitions(hal PUBLIC _DMA_ENABLED)
endif()

