# Microchip ATmega328P vendor drivers + AVR arch sources, aggregated into the
# one `hal` static library (mirrors src/vendor/stm32/CMakeLists.txt). avr-libc
# supplies the C runtime and startup, so there is no startup.s to link.
#
# Vendor IP drivers, gated on their CONFIG_DRV_* symbols.

set(HAL_SOURCES)

# Arch layer (src/arch/avr/, via the SRC_ARCH path the root CMakeLists sets).
if(CONFIG_DRV_INTERRUPT)
    list(APPEND HAL_SOURCES ${SRC_ARCH}/interrupt/interrupt.c)
endif()

# Vendor IP drivers.
if(CONFIG_DRV_GPIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c)
endif()
if(CONFIG_DRV_CLOCK)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c)
endif()
if(CONFIG_DRV_TIMER)
    # The ATmega328P timebase is Timer0-backed (a vendor peripheral, unlike
    # the Cortex-M SysTick); the general-purpose timer driver uses Timer1
    # and Timer2.
    list(APPEND HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/timebase/timebase.c
        ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer.c
    )
endif()
if(CONFIG_DRV_UART)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c)
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
if(CONFIG_DRV_CRC)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/crc/crc.c)
endif()
if(CONFIG_DRV_FLASH)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/flash/flash.c)
endif()

add_library(hal STATIC
    ${HAL_SOURCES}
    $<TARGET_OBJECTS:common>
)

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
    ${CMAKE_BINARY_DIR}
)
