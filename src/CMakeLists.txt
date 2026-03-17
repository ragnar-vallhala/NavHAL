include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_library(common OBJECT
${CMAKE_CURRENT_SOURCE_DIR}/utils/conversion.c
${CMAKE_CURRENT_SOURCE_DIR}/utils/util.c
${CMAKE_CURRENT_SOURCE_DIR}/utils/v_fs.c
${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/diskio.c
${CMAKE_CURRENT_SOURCE_DIR}/utils/fatfs/ff.c
)
