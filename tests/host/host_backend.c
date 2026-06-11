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
 * @file host_backend.c
 * @brief navtest output backend for the host build — writes to stdout.
 *
 * The on-target backend lives in tests/navtest_state.c and routes to UART2.
 * Selection is via the `NAVTEST_HOST` compile definition (set by
 * tests/host/CMakeLists.txt).
 */

#include "navtest/navtest.h"
#include <stdio.h>

void navtest_write(const char *s) { fputs(s, stdout); }
