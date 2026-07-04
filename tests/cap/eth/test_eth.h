/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
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

#ifndef TEST_CAP_ETH_H
#define TEST_CAP_ETH_H

#include "common/hal_features.h"
#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_CONFIG_DRV_ETH

void test_eth_rejects_null_args(void);
void test_eth_phy_id_readable(void);

extern const navtest_suite_t test_eth_suite;

#endif /* NAVHAL_CONFIG_DRV_ETH */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TEST_CAP_ETH_H */
