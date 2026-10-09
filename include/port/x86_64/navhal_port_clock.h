/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/navhal_port_clock.h
 * @brief x86-64 clock port header — no port-specific extras (intentionally empty).
 */

#ifndef NAVHAL_PORT_X86_64_CLOCK_H
#define NAVHAL_PORT_X86_64_CLOCK_H

/* Intentionally empty — the public hal_clock_* API is the whole surface. */


/* This port carries the deprecated clock names. The shims are static inline
 * wrappers over the API, so common/hal_clock.h includes them once that API
 * is declared -- from here they would be forwarding to nothing. */
#define NAVHAL_PORT_CLOCK_COMPAT 1

#endif /* NAVHAL_PORT_X86_64_CLOCK_H */
