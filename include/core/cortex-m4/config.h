#ifndef CORTEX_M4_CONFIG_H
#define CORTEX_M4_CONFIG_H

// Configuration options
// Each flag is only defined here if it was not already set via a compiler -D
// flag (e.g. from CMake's add_compile_definitions). This avoids redefinition
// warnings when both the build system and this header enable the same feature.
#ifndef _DMA_ENABLED
#define _DMA_ENABLED
#endif

#endif // CORTEX_M4_CONFIG_H