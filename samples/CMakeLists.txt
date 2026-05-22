# Sample selector. Samples are organised into three tiers (see
# samples/README.md):
#   portable/  — build on every port (Cortex-M4 and AVR);
#   cortex-m/  — need DMA / FPU / DWT / SDIO, Cortex-M only;
#   no_hal/    — register-level, no-HAL teaching code (per-MCU).
#
# AVAILABLE_SAMPLES holds the selectable short names; SAMPLE_DIRS holds the
# matching tier-relative directory for each (the two lists are parallel).

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
hal_dma_i2c
no_hal_flash
hal_flash
hal_uart_int
hal_uart_dma
hal_dma_polling_uart
hal_fpu
hal_crc
hal_sdio
hal_sdio_block
hal_fatfs_posix
hal_sdio_perf
hal_uart_dma_bridge
hal_spi_esp_bridge
hal_dwt
hal_blink_cpp
)
set(SAMPLE_DIRS
no_hal/01_no_hal_blink
portable/01_hal_blink
portable/02_hal_pupd
no_hal/03_no_hal_uart_tx
portable/03_hal_uart_tx
portable/04_hal_timer
cortex-m/05_hal_systick
portable/06_hal_blink_delay
portable/07_hal_pwm
no_hal/08_no_hal_pwm
cortex-m/09_hal_clock
no_hal/10_no_hal_i2c
portable/11_hal_i2c
cortex-m/19_hal_dma_i2c
no_hal/12_no_hal_flash
portable/13_hal_flash
portable/14_hal_uart_int
cortex-m/15_hal_uart_dma
cortex-m/16_hal_dma_polling_uart
cortex-m/17_hal_fpu
portable/18_hal_crc
cortex-m/20_hal_sdio
cortex-m/21_hal_sdio_block
cortex-m/22_hal_fatfs_posix
cortex-m/23_hal_sdio_perf
cortex-m/24_hal_uart_dma_bridge
portable/25_hal_spi_esp_bridge
cortex-m/26_hal_dwt
portable/27_hal_blink_cpp
)

# Check if sample is defined
if(NOT DEFINED SAMPLE)
  message(
    FATAL_ERROR "BUILD_STANDALONE_SAMPLES=ON requires SAMPLE=<sample_name>")
endif()

# Check for the sample asked to build
list(FIND AVAILABLE_SAMPLES ${SAMPLE} SAMPLE_INDEX)
if(${SAMPLE_INDEX} EQUAL -1)
  # Fallback: allow selecting by the tier-relative directory name
  # (e.g. "portable/01_hal_blink").
  list(FIND SAMPLE_DIRS ${SAMPLE} SAMPLE_INDEX)
endif()

if(${SAMPLE_INDEX} EQUAL -1)
  message(
    FATAL_ERROR "Sample ${SAMPLE} not found. Available samples: ${AVAILABLE_SAMPLES}\nAvailable dirs: ${SAMPLE_DIRS}")
endif()

list(GET SAMPLE_DIRS ${SAMPLE_INDEX} SAMPLE_DIR)
message(STATUS "Building sample '${SAMPLE}' from directory '${SAMPLE_DIR}'")

add_subdirectory(${SAMPLE_DIR})
