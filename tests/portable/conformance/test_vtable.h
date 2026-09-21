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

/**
 * @file test_vtable.h
 * @brief Vtable completeness — did this port actually implement the HAL?
 *
 * @details
 * The driver-vtable design (@ref roadmap_m9) makes "add a vendor" mean "fill
 * in an ops table". This suite is what makes that a promise rather than a
 * hope: every table the build links must have every entry filled in.
 *
 * It exists because the compiler will not do this. The design assumed
 * @c -Wmissing-field-initializers would catch a partially-filled table, but
 * GCC does not warn about missing fields in a *designated* initializer --
 * which is how every ops table in the tree is written -- not under @c -Wall
 * and not under @c -Wextra. A port that omits an entry gets a NULL there and
 * a silent build; the first sign of trouble is a jump to address zero on
 * hardware.
 *
 * Unlike test_conformance.c this suite is deliberately **white-box**: it
 * includes the internal `include/internal/hal_*_ops.h` headers, because the
 * thing under test is the internal contract itself.
 */
#ifndef TEST_VTABLE_H
#define TEST_VTABLE_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const navtest_suite_t test_vtable_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TEST_VTABLE_H */
