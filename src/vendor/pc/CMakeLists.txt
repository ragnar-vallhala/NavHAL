# PC (x86) platform drivers. Standard PC peripherals (16550 UART; 8254 PIT /
# 8259 PIC to follow). The x86-64 startup (multiboot2 header + long-mode entry)
# lives under src/arch/x86_64/startup and is pulled in here so it lands in the
# aggregated `hal` library, mirroring how the STM32 vendor pulls in its arch
# startup.

set(HAL_SOURCES)

if(CONFIG_DRV_UART)
    # uart.c mirrors console output to the VGA text terminal (vga.c) so the PC
    # screen shows what goes out on serial.
    list(APPEND HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/vga/vga.c)
endif()
if(CONFIG_DRV_CLOCK)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c)
endif()
if(CONFIG_DRV_TIMER)
    # TSC-backed timebase (delay/millis), polled. The general-purpose hal_timer_*
    # (PIT channels) needs IRQs and lands with the interrupt slice.
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/timer/timebase.c)
endif()
if(CONFIG_DRV_INTERRUPT)
    # PIC driver (hal_interrupt_*) is vendor; the IDT + ISR entry stubs are the
    # x86_64 CPU arch layer (mirrors NVIC-in-arch on Cortex-M).
    list(APPEND HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/interrupt/interrupt.c
        ${SRC_ARCH}/interrupt/idt.c
        ${SRC_ARCH}/interrupt/isr.s
    )
    # PS/2 keyboard feeds the screen-terminal input path in uart.c.
    if(CONFIG_DRV_UART)
        list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ps2/keyboard.c)
    endif()
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
    ${CMAKE_CURRENT_SOURCE_DIR}   # shared pc_io.h (port I/O + TSC)
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
    ${CMAKE_BINARY_DIR}
)
