include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/conversion.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/freestanding.c
)

# Not a driver: the console dispatches to whichever transport the board routed it
# to, so it is always available and gated on nothing. The TEST build picks it up
# through its own file(GLOB src/common/*.c) instead -- see the note below.
if(NOT TEST)
    list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_console.c)
endif()

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
    if(CONFIG_DRV_I2C_DMA)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_i2c_dma.c)
    endif()
    if(CONFIG_DRV_UART_DMA)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_uart_dma.c)
    endif()
    if(CONFIG_DRV_INTERRUPT)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_interrupt.c)
    endif()
    if(CONFIG_DRV_TIMEBASE)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_timebase.c)
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
    if(CONFIG_BOOT_SNIFFER)
        list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_boot.c)
    endif()
endif()

# Bootloader crypto: exactly one backend, and only its own sources. Compiling a
# backend that is not selected would put a second hal_boot_ed25519_verify in the
# link, so the choice is enforced here as well as in Kconfig.
if(CONFIG_BOOT_CRYPTO)
    set(CRYPTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils/crypto)
    if(CONFIG_BOOT_ED25519_MONOCYPHER)
        list(APPEND COMMON_SOURCES
            ${CRYPTO_DIR}/backend_monocypher.c
            ${CRYPTO_DIR}/monocypher/monocypher.c
            ${CRYPTO_DIR}/monocypher/monocypher-ed25519.c)
        set(CRYPTO_INCLUDES ${CRYPTO_DIR}/monocypher)
    elseif(CONFIG_BOOT_ED25519_TWEETNACL)
        list(APPEND COMMON_SOURCES
            ${CRYPTO_DIR}/backend_tweetnacl.c
            ${CRYPTO_DIR}/tweetnacl/tweetnacl.c)
        set(CRYPTO_INCLUDES ${CRYPTO_DIR}/tweetnacl)
    elseif(CONFIG_BOOT_ED25519_COMPACT25519)
        list(APPEND COMMON_SOURCES
            ${CRYPTO_DIR}/backend_compact25519.c
            ${CRYPTO_DIR}/compact25519/compact_ed25519.c
            ${CRYPTO_DIR}/compact25519/compact_wipe.c
            ${CRYPTO_DIR}/compact25519/c25519/ed25519.c
            ${CRYPTO_DIR}/compact25519/c25519/edsign.c
            ${CRYPTO_DIR}/compact25519/c25519/f25519.c
            ${CRYPTO_DIR}/compact25519/c25519/fprime.c
            ${CRYPTO_DIR}/compact25519/c25519/sha512.c)
        set(CRYPTO_INCLUDES ${CRYPTO_DIR}/compact25519)
    else()
        message(FATAL_ERROR
            "CONFIG_BOOT_CRYPTO is set but no Ed25519 backend is selected")
    endif()
endif()

if(CONFIG_DRV_SDIO)
    list(APPEND COMMON_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/v_fs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/diskio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/ff.c
    )
    # Only with LFN on: it carries the code-page conversion tables, and with
    # short names FatFs never calls them.
    if(CONFIG_FS_LFN)
        list(APPEND COMMON_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/ffunicode.c
        )
    endif()
endif()

add_library(common OBJECT ${COMMON_SOURCES})

# The shared public driver layers include the active port header
# (common/hal_*.h pulls in navhal_port_*.h), so `common` needs the same
# port/family/board search paths the vendor `hal` library uses. These vars
# are set by the root CMakeLists before add_subdirectory(src).
target_include_directories(common PRIVATE
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${CMAKE_BINARY_DIR}   # generated board.h
    ${CMAKE_BINARY_DIR}

    ${CRYPTO_INCLUDES})
