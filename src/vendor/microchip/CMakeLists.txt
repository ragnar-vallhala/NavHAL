# Microchip ATmega328P vendor drivers + AVR arch sources, aggregated into the
# one `hal` static library (mirrors src/vendor/stm32/CMakeLists.txt). avr-libc
# supplies the C runtime and startup, so there is no startup.s to link.
#
# M6 PR4 fills in the vendor driver sources (gpio, clock, timer, timebase,
# uart, ...); this file currently wires the AVR arch interrupt backend.

set(HAL_SOURCES)

# Arch layer (src/arch/avr/, via the SRC_ARCH path the root CMakeLists sets).
if(CONFIG_DRV_INTERRUPT)
    list(APPEND HAL_SOURCES ${SRC_ARCH}/interrupt/interrupt.c)
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
