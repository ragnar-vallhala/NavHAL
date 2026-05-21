/**
 * @file core/cortex-m4/dwt.h
 * @brief Cortex-M4 cycle-counter port header.
 *
 * @details
 * The public prototypes live in @c common/hal_dwt.h, which includes this
 * header. This file is currently the deprecated-name compatibility shim only;
 * its presence preserves the @c #include "core/cortex-m4/dwt.h" path used by
 * existing source files.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DWT_H
#define CORTEX_M4_DWT_H

#include "common/hal_dwt.h"


/* Deprecated pre-standardization DWT names — removed in M5. */
#include "compat/dwt_compat.h"

#endif /* CORTEX_M4_DWT_H */
