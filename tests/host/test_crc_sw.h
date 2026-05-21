#ifndef TEST_HOST_CRC_SW_H
#define TEST_HOST_CRC_SW_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_crc_sw_init_rejects_null(void);
void test_crc_sw_empty_returns_init(void);
void test_crc_sw_single_byte(void);
void test_crc_sw_mpeg2_reference_vector(void);
void test_crc_sw_accumulate_matches_compute(void);
void test_crc_sw_reset_restores_init(void);

extern const navtest_suite_t test_crc_sw_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
