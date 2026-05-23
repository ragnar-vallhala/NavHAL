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

include_directories(
  ${NAVHAL_ROOT}/include
  # The host build always targets the Cortex-M4 / STM32F4 port (it
  # compiles a subset of those sources with the system compiler), so
  # we don't drive these paths from Kconfig here — the embedded build
  # in ${NAVHAL_ROOT}/CMakeLists.txt does, via ARCH_ISA / FAMILY.
  ${NAVHAL_ROOT}/include/port/cortex-m4
  ${NAVHAL_ROOT}/src/vendor/stm32/family/stm32f4/include
)

add_executable(tests_host
  main.c
  host_backend.c
  test_conversion.c
  test_crc_sw.c
  test_gpio_encoding.c
  test_hal_status.c

  # shared harness
  ${NAVHAL_ROOT}/tests/navtest_state.c

  # source under test — pure-logic only (no register access)
  ${NAVHAL_ROOT}/src/utils/conversion.c
  ${NAVHAL_ROOT}/src/vendor/stm32/crc/crc.c
)

enable_testing()
add_test(NAME tests_host COMMAND tests_host)
