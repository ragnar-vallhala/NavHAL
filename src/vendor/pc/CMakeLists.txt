# PC (x86) platform drivers. Standard PC peripherals (16550 UART; 8254 PIT /
# 8259 PIC to follow). The x86-64 startup (multiboot2 header + long-mode entry)
# lives under src/arch/x86_64/startup and is pulled in here so it lands in the
# aggregated `hal` library, mirroring how the STM32 vendor pulls in its arch
# startup.

set(HAL_SOURCES)

if(CONFIG_DRV_UART)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c)
endif()

# Arch startup (asm): multiboot2 header, 32-bit stub -> long mode -> main().
list(APPEND HAL_SOURCES ${SRC_ARCH}/startup/startup.s)

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
