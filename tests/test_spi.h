#ifndef TEST_SPI_H
#define TEST_SPI_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_spi_init_config(void);
void test_hal_spi_init_returns_ok(void);
void test_hal_spi_init_rejects_null_config(void);
void test_hal_spi_transmit_rejects_null_data(void);
void test_hal_spi_receive_rejects_null_data(void);
void test_hal_spi_transmit_receive_rejects_null_data(void);
void test_hal_spi_init_cpol_low_cpha_1edge(void);

extern const navtest_suite_t test_spi_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_SPI_H
