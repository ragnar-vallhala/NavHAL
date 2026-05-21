#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H


#ifdef __cplusplus
extern "C" {
#endif
// Configuration options
// Each flag is only defined here if it was not already set via a compiler -D
// flag (e.g. from CMake's add_compile_definitions). This avoids redefinition
// warnings when both the build system and this header enable the same feature.
#ifndef _FPU_ENABLED
#define _FPU_ENABLED
#endif

#ifndef _DMA_ENABLED
#define _DMA_ENABLED
#endif

#ifndef _CRC_HW_ENABLED
#define _CRC_HW_ENABLED
#endif

#ifndef _DWT_ENABLED
#define _DWT_ENABLED
#endif

#ifndef _SDIO_ENABLED
#define _SDIO_ENABLED
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // NAVHAL_PORT_CONFIG_H