set(AVAILABLE_SAMPLES 
no_hal_blink 
hal_blink
hal_pupd
no_hal_uart_tx
hal_uart_tx
hal_timer
hal_systick
hal_blink_delay
hal_pwm
no_hal_pwm
hal_clock
no_hal_i2c
hal_i2c
)
set(SAMPLE_DIRS 
01_no_hal_blink 
01_hal_blink
02_hal_pupd
03_no_hal_uart_tx
03_hal_uart_tx
04_hal_timer
05_hal_systick
06_hal_blink_delay
07_hal_pwm
08_no_hal_pwm
09_hal_clock
10_no_hal_i2c
11_hal_i2c

)

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
