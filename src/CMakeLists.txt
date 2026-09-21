include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/conversion.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/freestanding.c
)

# Shared public driver layers (M9 driver vtable): validate + dispatch to the
# active backend's ops table. In a normal (sample/firmware) build these are
# gated on the same CONFIG_DRV_* symbols as the backends, so a firmware that
# omits a driver omits its public layer too. The TEST build instead globs every
# backend regardless of config (it exercises all drivers on the arch), so it
# picks these layers up the same way — via file(GLOB src/common/*.c) in the
# root CMakeLists TEST block — and must NOT also get them here, or the public
# symbols would be defined twice.
if(NOT TEST)
    if(CONFIG_DRV_GPIO)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_gpio.c)
    endif()
    if(CONFIG_DRV_CRC)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_crc.c)
    endif()
    if(CONFIG_DRV_CLOCK)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_clock.c)
    endif()
    if(CONFIG_DRV_FLASH)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_flash.c)
    endif()
    if(CONFIG_DRV_PWM)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_pwm.c)
    endif()
    if(CONFIG_DRV_UART)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_uart.c)
    endif()
    if(CONFIG_DRV_I2C)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_i2c.c)
    endif()
    if(CONFIG_DRV_SPI)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_spi.c)
    endif()
    if(CONFIG_DRV_TIMER)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_timer.c)
    endif()
    if(CONFIG_DRV_UART_DMA)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_uart_dma.c)
    endif()
    if(CONFIG_DRV_ADC)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_adc.c)
    endif()
    if(CONFIG_DRV_RESET)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_reset.c)
    endif()
    if(CONFIG_DRV_WATCHDOG)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_watchdog.c)
    endif()
    if(CONFIG_DRV_WWDG)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_wwdg.c)
    endif()
endif()

if(CONFIG_DRV_SDIO)
    list(APPEND COMMON_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/v_fs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/diskio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/ff.c
    )
endif()

add_library(common OBJECT ${COMMON_SOURCES})

# The shared public driver layers include the active port header
# (common/hal_*.h pulls in navhal_port_*.h), so `common` needs the same
# port/family/board search paths the vendor `hal` library uses. These vars
# are set by the root CMakeLists before add_subdirectory(src).
target_include_directories(common PRIVATE
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
    ${CMAKE_BINARY_DIR}
)
