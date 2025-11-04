include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_library(common
${CMAKE_CURRENT_SOURCE_DIR}/conversion.c
${CMAKE_CURRENT_SOURCE_DIR}/util.c
)
