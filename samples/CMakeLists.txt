set(AVAILABLE_SAMPLES 
no_hal_blink 
hal_blink)
set(SAMPLE_DIRS 
01_no_hal_blink 
01_hal_blink)

# Check if sample is defined
if(NOT DEFINED SAMPLE)
  message(
    FATAL_ERROR "BUILD_STANDALONE_SAMPLES=ON requires SAMPLE=<sample_name>")
endif()

# Check for the sample asked to build
list(FIND AVAILABLE_SAMPLES ${SAMPLE} SAMPLE_INDEX)
if(${SAMPLE_INDEX} EQUAL -1)
  message(
    FATAL_ERROR "Sample ${SAMPLE} not found. Available: ${AVAILABLE_SAMPLES}")
endif()

list(GET SAMPLE_DIRS ${SAMPLE_INDEX} SAMPLE_DIR)
message(STATUS "Building sample '${SAMPLE}' from directory '${SAMPLE_DIR}'")

add_subdirectory(${SAMPLE_DIR})
