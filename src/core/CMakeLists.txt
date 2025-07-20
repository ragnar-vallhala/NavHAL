message(STATUS ${CMAKE_CURRENT_SOURCE_DIR}/cortex-m4/gpio/gpio.c)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(hal 
STATIC
${CMAKE_CURRENT_SOURCE_DIR}/cortex-m4/gpio/gpio.c 
${CMAKE_CURRENT_SOURCE_DIR}/cortex-m4/startup/startup.s 
)