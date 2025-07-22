include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(hal 
STATIC
${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio.c 
${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.s 
${CMAKE_CURRENT_SOURCE_DIR}/clock/clock.c
${CMAKE_CURRENT_SOURCE_DIR}/uart/uart.c
)

target_link_libraries(hal gcc)