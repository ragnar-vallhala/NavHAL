/**
 * @file tests/test_crc.h
 * @brief CRC test function declarations for NavTest.
 */

#ifndef TEST_CRC_H
#define TEST_CRC_H

void test_crc_empty_returns_init(void);
void test_crc_single_byte(void);
void test_crc_known_vector(void);
void test_crc_accumulate_matches_compute(void);
void test_crc_reset_restores_init(void);

#endif /* TEST_CRC_H */
