# Microchip ATmega328P vendor drivers + AVR arch sources, aggregated into the
# one `hal` static library (mirrors src/vendor/stm32/CMakeLists.txt). avr-libc
# supplies the C runtime and startup, so there is no startup.s to link.
#
# The acceptance-slice drivers (gpio, clock, timebase, uart) are in place;
# timer/i2c/spi/pwm/crc follow in a later M6 pass.

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
    # the Cortex-M SysTick). The general-purpose timer driver is not in the
    # acceptance slice yet.
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/timebase/timebase.c)
endif()
if(CONFIG_DRV_UART)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c)
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
