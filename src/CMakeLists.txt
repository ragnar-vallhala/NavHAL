include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/conversion.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/util.c
)

# Shared public driver layers (M9 driver vtable): validate + dispatch to the
# active vendor's ops table. Gated on the same CONFIG_DRV_* symbols as the
# vendor backends so a firmware that omits a driver omits its public layer too.
if(CONFIG_DRV_GPIO)
    list(APPEND COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/hal_gpio.c)
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
