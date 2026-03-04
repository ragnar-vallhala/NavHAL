#ifndef CORTEX_M4_FPU_H
#define CORTEX_M4_FPU_H
/**
 * @brief Enable the hardware Floating Point Unit (FPU).
 *
 * Enables CP10 and CP11 coprocessors in the CPACR register
 * and sets the FPU to use lazy stacking.
 */
void hal_fpu_enable(void);
#endif // CORTEX_M4_FPU_H