/*
 * Copyright (C) 2026 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file test_adc_driver.c
 * @brief Deep host (SIL) tests for adc.c against simulated MMIO.
 *
 * @details
 * Two things the on-target tiers cannot check here. The emulator does not
 * retain writes to the ADC's sample-time registers, so a PIL run cannot tell a
 * configured sample time from a reset one; and a real converter cannot be held
 * mid-conversion long enough to reproduce the stale-EOC case on purpose. Both
 * are ordinary memory under host_mmio, so both are deterministic here.
 *
 * `SR` never gains EOC on the host, so every read below ends in
 * ::HAL_ERR_TIMEOUT after its bounded spin. That is the point in one case and
 * incidental in the other, and it is noted where it matters.
 */

#include "host_mmio.h"
#include "navhal_port_adc.h"
#include "family/adc_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

/* Field accessors mirroring the driver: SMPR2 holds channels 0-9, SMPR1 holds
 * 10-18, three bits each. Written out rather than shared so a wrong shift in the
 * driver cannot cancel itself out against the same wrong shift here. */
static uint32_t smp_of(uint8_t channel) {
  volatile ADC_Reg_Typedef *a = GET_ADCx_BASE(0);
  return (channel < 10u) ? ((a->SMPR2 >> (3u * channel)) & 7u)
                         : ((a->SMPR1 >> (3u * (channel - 10u))) & 7u);
}

/* The configured sample time has to reach the silicon, in the register half the
 * channel belongs to -- easy to get right in one direction and wrong in the
 * other. 28 cycles encodes as 2, which is neither the reset value (0) nor the
 * default (7), so this also fails if the field is ignored or pinned. */
void test_host_adc_sample_time_lands_in_smpr(void) {
  host_mmio_reset();
  const hal_adc_config_t cfg = {.resolution = HAL_ADC_RES_12BIT,
                                .sample_time = HAL_ADC_SAMPLE_28_CYCLES};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_adc_init(HAL_ADC_1, &cfg));

  uint16_t v = 0u;
  (void)hal_adc_read(HAL_ADC_1, 3u, &v); /* times out; SMPR is written first */
  TEST_ASSERT_EQUAL_UINT32(2u, smp_of(3u));

  (void)hal_adc_read(HAL_ADC_1, 12u, &v);
  TEST_ASSERT_EQUAL_UINT32(2u, smp_of(12u));
}

/* A zero-initialised config, and a NULL one, must both give the longest sample
 * time rather than the silicon's 3-cycle reset value: a sample too short for the
 * source reads low and reports nothing, so the safe end is the one that has to
 * be the default. */
void test_host_adc_default_sample_time_is_the_longest(void) {
  host_mmio_reset();
  const hal_adc_config_t zeroed = {0};
  uint16_t v = 0u;

  hal_adc_init(HAL_ADC_1, &zeroed);
  (void)hal_adc_read(HAL_ADC_1, 0u, &v);
  TEST_ASSERT_EQUAL_UINT32(7u, smp_of(0u));

  host_mmio_reset();
  hal_adc_init(HAL_ADC_1, NULL);
  (void)hal_adc_read(HAL_ADC_1, 1u, &v);
  TEST_ASSERT_EQUAL_UINT32(7u, smp_of(1u));
}

/* Regression. A read that times out leaves its conversion running, and that
 * conversion finishes and latches EOC afterwards. A read that trusts EOC then
 * skips its wait and returns the abandoned sample as HAL_OK -- a stale value
 * reported as a fresh one, which is the worst shape a sensor read can take.
 *
 * Forge exactly that: EOC set, a value parked in DR, nobody having collected
 * it. Here no new conversion can complete, so the honest answer is a timeout
 * with `out` untouched. The bug's answer is HAL_OK and 0x0ABC. */
void test_host_adc_read_does_not_trust_a_stale_eoc(void) {
  host_mmio_reset();
  hal_adc_init(HAL_ADC_1, NULL);

  volatile ADC_Reg_Typedef *a = GET_ADCx_BASE(0);
  a->DR = 0x0ABCu;
  host_reg_set((uintptr_t)&a->SR, ADC_SR_EOC);
  TEST_ASSERT_BITS_HIGH(ADC_SR_EOC, a->SR);

  uint16_t v = 0xFFFFu;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_TIMEOUT,
                           (uint32_t)hal_adc_read(HAL_ADC_1, 0u, &v));
  TEST_ASSERT_EQUAL_UINT32(0xFFFFu, (uint32_t)v);
}

/* An out-of-range sample time is rejected at init rather than indexing the
 * driver's translation table out of bounds. */
void test_host_adc_init_rejects_bad_sample_time(void) {
  host_mmio_reset();
  const hal_adc_config_t cfg = {.resolution = HAL_ADC_RES_12BIT,
                                .sample_time = (hal_adc_sample_time_t)99};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_adc_init(HAL_ADC_1, &cfg));
}

NAVTEST_CASE_DECL(test_host_adc_sample_time_lands_in_smpr);
NAVTEST_CASE_DECL(test_host_adc_default_sample_time_is_the_longest);
NAVTEST_CASE_DECL(test_host_adc_read_does_not_trust_a_stale_eoc);
NAVTEST_CASE_DECL(test_host_adc_init_rejects_bad_sample_time);

static const navtest_case_t adc_driver_cases[] = {
    NAVTEST_CASE(test_host_adc_sample_time_lands_in_smpr),
    NAVTEST_CASE(test_host_adc_default_sample_time_is_the_longest),
    NAVTEST_CASE(test_host_adc_read_does_not_trust_a_stale_eoc),
    NAVTEST_CASE(test_host_adc_init_rejects_bad_sample_time),
};

const navtest_suite_t test_adc_driver_suite = {
    .name = "ADC DRIVER (host)",
    .cases = adc_driver_cases,
    .count = sizeof(adc_driver_cases) / sizeof(adc_driver_cases[0]),
    .between = NULL,
};
