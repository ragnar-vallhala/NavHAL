# ACME reference port. GPIO only -- the point of this port is how little a
# vendor has to supply, so it does not grow a driver without a reason.
set(HAL_SOURCES)

if(CONFIG_DRV_GPIO)
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c)
endif()

# The shared public layer (src/common/hal_*.c) comes in as objects, same as
# every other vendor: the vendor library is the backend plus that layer.
add_library(hal STATIC
    ${HAL_SOURCES}
    $<TARGET_OBJECTS:common>
)

target_link_libraries(hal gcc)

# INCLUDE_PORT / INCLUDE_FAMILY / SRC_BOARD are set by the root CMakeLists
# from the Kconfig identity strings, so a vendor names no paths of its own.
target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${INCLUDE_PORT}
    ${INCLUDE_FAMILY}
    ${SRC_BOARD}
    ${CMAKE_BINARY_DIR}
)
