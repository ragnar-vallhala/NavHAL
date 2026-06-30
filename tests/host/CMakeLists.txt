# Standalone host-test build — uses the system gcc (no arm-none-eabi toolchain).
#
# Run from anywhere via:
#   cmake -B build-host -S tests/host
#   cmake --build build-host
#   ./build-host/tests_host
#
# The host build deliberately does NOT include the root NavHAL CMakeLists,
# because the root sets the embedded toolchain prefix and architecture flags.
# Instead, it picks targeted source files from the tree below and compiles
# them with the host compiler. NAVTEST_HOST switches the navtest output
# backend to stdout (see tests/host/host_backend.c).

cmake_minimum_required(VERSION 3.20)
project(navhal_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(NAVHAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -O0 -g)
add_compile_definitions(NAVTEST_HOST)

enable_testing()

# -------------------------------------------------------------------------
# tests_host — pure-logic SIL suite (Cortex-M4 / STM32F4 headers).
# -------------------------------------------------------------------------
add_executable(tests_host
  main.c
  host_backend.c
  test_conversion.c
  test_crc_sw.c
  test_gpio_encoding.c
  test_hal_status.c

  ${NAVHAL_ROOT}/tests/navtest_state.c

  # source under test — pure-logic only (no register access)
  ${NAVHAL_ROOT}/src/utils/conversion.c
  ${NAVHAL_ROOT}/src/vendor/stm32/crc/crc.c
)
target_include_directories(tests_host PRIVATE
  ${NAVHAL_ROOT}/include
  ${NAVHAL_ROOT}/include/port/cortex-m4
  ${NAVHAL_ROOT}/src/vendor/stm32/family/stm32f4/include
)
add_test(NAME tests_host COMMAND tests_host)

# -------------------------------------------------------------------------
# tests_host_drivers — deep SIL suite that runs the *real* STM32F7 drivers
# against a simulated MMIO backing store (host_mmio.c). Catches register/
# control-flow bugs the pure-logic suite can't. Cortex-M7 / STM32F7 headers.
# -------------------------------------------------------------------------
add_executable(tests_host_drivers
  main_drivers.c
  host_backend.c
  host_mmio.c
  host_stubs.c
  test_gpio_driver.c
  test_uart_driver.c
  test_i2c_driver.c
  test_spi_driver.c

  ${NAVHAL_ROOT}/tests/navtest_state.c

  # real driver sources under test
  ${NAVHAL_ROOT}/src/vendor/stm32/gpio/gpio.c
  ${NAVHAL_ROOT}/src/vendor/stm32/clock/clock_f7.c
  ${NAVHAL_ROOT}/src/vendor/stm32/uart/uart_f7.c
  ${NAVHAL_ROOT}/src/vendor/stm32/i2c/i2c_f7.c
  ${NAVHAL_ROOT}/src/vendor/stm32/spi/spi_f7.c
)
target_include_directories(tests_host_drivers PRIVATE
  ${NAVHAL_ROOT}/include
  ${NAVHAL_ROOT}/include/port/cortex-m7
  ${NAVHAL_ROOT}/src/vendor/stm32/family/stm32f7/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)
# The drivers cast 32-bit peripheral addresses to pointers; on a 64-bit host
# that's an int<->pointer width mismatch (the mmap places them in low memory,
# so the values are valid). Silence the expected host-only warnings.
target_compile_options(tests_host_drivers PRIVATE
  -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast)
add_test(NAME tests_host_drivers COMMAND tests_host_drivers)
