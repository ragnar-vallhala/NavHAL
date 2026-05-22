/**
 * @file port/avr/navhal_port_config.h
 * @brief AVR / ATmega328P build-time feature flags.
 *
 * @details
 * Counterpart of the Cortex-M4 navhal_port_config.h. The ATmega328P has
 * none of the optional peripherals the HAL gates on — no DMA, no hardware
 * FPU, no DWT-style cycle counter, no SDIO, no hardware CRC — so this
 * header defines no `_*_ENABLED` flags. The DMA / SDIO API headers
 * therefore collapse to nothing, and `NAVHAL_HAS_*` resolve to 0.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_CONFIG_H
#define NAVHAL_PORT_CONFIG_H

/* Intentionally empty: the ATmega328P exposes no gated optional peripheral. */

#endif /* NAVHAL_PORT_CONFIG_H */
