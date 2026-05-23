/**
 * @file hal_config.h
 * @brief Portable HAL configuration-macro entry point.
 *
 * @details
 * Exposes the build-time feature flags (@c _FPU_ENABLED, @c _DMA_ENABLED,
 * etc.) used by the rest of the HAL. The actual macro definitions live in
 * the per-port @c config.h for now; they will be relocated to a Kconfig-
 * generated @c navhal_target.h in WI4.3.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "navhal_port_config.h"

#endif /* HAL_CONFIG_H */
