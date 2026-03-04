#ifndef HAL_FPU_H
#define HAL_FPU_H

#include <stdint.h>

/**
 * @brief Enable the hardware Floating Point Unit (FPU).
 *
 * Enables CP10 and CP11 coprocessors in the CPACR register
 * and sets the FPU to use lazy stacking.
 */
void hal_fpu_enable(void);

#endif // HAL_FPU_H
