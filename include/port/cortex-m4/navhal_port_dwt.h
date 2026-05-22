/**
 * @file port/cortex-m4/navhal_port_dwt.h
 * @brief Cortex-M4 cycle-counter port header.
 *
 * @details
 * The public prototypes live in @c common/hal_dwt.h, which includes this
 * header. This file is currently the deprecated-name compatibility shim only;
 * its presence preserves the @c navhal_port_dwt.h include path used by
 * existing source files.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_DWT_H
#define NAVHAL_PORT_DWT_H

#include "common/hal_dwt.h"


/* Deprecated pre-standardization DWT names — retained as a backward-compat alias. */
#include "compat/dwt_compat.h"

#endif /* NAVHAL_PORT_DWT_H */
