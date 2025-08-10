#ifndef CORTEX_M4_INTERRUPT_H
#define CORTEX_M4_INTERRUPT_H

#include "core/cortex-m4/interrupt_reg.h"
#include <stdint.h>
int8_t
hal_enable_interrupt(IRQn_Type interrupt); // return 0 if enable success else -1
int8_t hal_disable_interrupt(
    IRQn_Type interrupt); // return 0 if enable success else -1

void hal_clear_interrupt_flag(
    IRQn_Type interrupt); // [TODO] add later specific to the port
void hal_interrupt_attach_callback(IRQn_Type interrupt, void (*callback)(void));
void hal_interrupt_detach_callback(IRQn_Type interrupt);
void hal_handle_interrupt(IRQn_Type interrupt);
void hal_set_interrupt_priority(IRQn_Type interrupt, uint8_t priority); //[TODO]
uint8_t hal_get_interrupt_priority(IRQn_Type interrupt);                //[TODO]
int hal_is_interrupt_pending(
    IRQn_Type interrupt); //[TODO]  // returns 1 if pending, 0 if not
void hal_enable_global_interrupts(uint32_t state); //[TODO]
uint32_t hal_disable_global_interrupts(void);      //[TODO]
void hal_clear_all_pending_interrupts(void);       //[TODO]

#endif //! CORTEX_M4_INTERRUPT_H
