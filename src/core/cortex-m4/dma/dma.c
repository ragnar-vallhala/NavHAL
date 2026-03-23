/**
 * @file src/core/cortex-m4/dma/dma.c
 * @brief DMA driver implementation for STM32F4.
 *
 * @details
 * Implements clock enable, stream configuration, start/stop, flag polling,
 * and flag clearing for DMA1 and DMA2 on STM32F4.
 *
 * Only compiled when @c _DMA_ENABLED is defined.
 *
 * @ingroup HAL_DMA
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/config.h"
#ifdef _DMA_ENABLED

#include "core/cortex-m4/dma.h"
#include "core/cortex-m4/dma_reg.h"
#include "core/cortex-m4/interrupt.h"
#include "core/cortex-m4/rcc_reg.h"
#include <stdint.h>

/*---------------------------------------------------------------------------
 * Internal helpers
 *---------------------------------------------------------------------------*/

/** Return a pointer to the DMA controller based on cfg->controller. */
static inline DMA_Typedef *_get_dma(const dma_config_t *cfg) {
  return (cfg->controller == DMA_CONTROLLER_2) ? DMA2 : DMA1;
}

/** Return a pointer to the specific stream register block. */
static inline DMA_Stream_Typedef *_get_stream(const dma_config_t *cfg) {
  return &_get_dma(cfg)->STREAM[cfg->stream & 0x7U];
}

/**
 * @brief Clear all interrupt flags for a given stream.
 *
 * Writes to LIFCR or HIFCR depending on the stream index.
 */
static void _clear_flags(DMA_Typedef *dma, uint8_t stream) {
  uint32_t mask =
      (DMA_ISR_TCIF(stream) | DMA_ISR_HTIF(stream) | DMA_ISR_TEIF(stream) |
       DMA_ISR_DMEIF(stream) | DMA_ISR_FEIF(stream));

  *DMA_IFCR_REG(dma, stream) = mask;
}

/*---------------------------------------------------------------------------
 * Public API
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize a DMA stream.
 */
void dma_init(const dma_config_t *cfg) {
  /* 1. Enable peripheral clock */
  if (cfg->controller == DMA_CONTROLLER_1)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
  else
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

  DMA_Stream_Typedef *s = _get_stream(cfg);
  DMA_Typedef *d = _get_dma(cfg);

  /* 2. Disable the stream and wait until it is off */
  s->CR &= ~DMA_SxCR_EN;
  while (s->CR & DMA_SxCR_EN)
    ;

  /* 3. Clear any lingering interrupt flags */
  _clear_flags(d, cfg->stream);

  /* 4. Set peripheral and memory addresses */
  if (cfg->direction == DMA_DIR_M2P) {
    s->PAR = cfg->dst_addr;  /* peripheral = destination */
    s->M0AR = cfg->src_addr; /* memory     = source      */
  } else {
    s->PAR = cfg->src_addr;  /* peripheral = source      */
    s->M0AR = cfg->dst_addr; /* memory     = destination */
  }

  /* 5. Number of data items */
  s->NDTR = cfg->data_count;

  /* 6. Build CR value */
  uint32_t cr = 0;

  /* Channel */
  cr |= DMA_SxCR_CHSEL(cfg->channel);

  /* Direction */
  switch (cfg->direction) {
  case DMA_DIR_P2M:
    cr |= DMA_SxCR_DIR_P2M;
    break;
  case DMA_DIR_M2P:
    cr |= DMA_SxCR_DIR_M2P;
    break;
  case DMA_DIR_M2M:
    cr |= DMA_SxCR_DIR_M2M;
    break;
  }

  /* Priority */
  cr |= ((uint32_t)cfg->priority << DMA_SxCR_PL_POS) & DMA_SxCR_PL_MASK;

  /* Memory data size */
  cr |= ((uint32_t)cfg->data_width << DMA_SxCR_MSIZE_POS) & DMA_SxCR_MSIZE_MASK;

  /* Peripheral data size */
  cr |= ((uint32_t)cfg->data_width << DMA_SxCR_PSIZE_POS) & DMA_SxCR_PSIZE_MASK;

  /* Increment modes:
   *   In M2P: src = memory (increment), dst = peripheral (no increment)
   *   In P2M: src = peripheral (no increment), dst = memory (increment)
   */
  if (cfg->direction == DMA_DIR_M2P) {
    if (cfg->src_inc)
      cr |= DMA_SxCR_MINC;
    if (cfg->dst_inc)
      cr |= DMA_SxCR_PINC;
  } else {
    if (cfg->dst_inc)
      cr |= DMA_SxCR_MINC;
    if (cfg->src_inc)
      cr |= DMA_SxCR_PINC;
  }

  /* Circular mode */
  if (cfg->circular)
    cr |= DMA_SxCR_CIRC;

  /* Peripheral flow control */
  if (cfg->pfctrl)
    cr |= DMA_SxCR_PFCTRL;

  /* Burst modes */
  cr |= ((uint32_t)cfg->mburst << DMA_SxCR_MBURST_POS) & DMA_SxCR_MBURST_MASK;
  cr |= ((uint32_t)cfg->pburst << DMA_SxCR_PBURST_POS) & DMA_SxCR_PBURST_MASK;

  /* Transfer-complete interrupt enable (useful for ISR-driven usage) */
  cr |= DMA_SxCR_TCIE;

  s->CR = cr;

  /* 7. Build FCR value */
  uint32_t fcr = 0;
  if (cfg->fifo_mode) {
    fcr |= DMA_SxFCR_DMDIS;
    fcr |= ((uint32_t)cfg->fifo_threshold << DMA_SxFCR_FTH_POS) &
           DMA_SxFCR_FTH_MASK;
  }
  s->FCR = fcr;
}

/**
 * @brief Enable a DMA stream to begin the transfer.
 */
void dma_start(const dma_config_t *cfg) {
  DMA_Typedef *d = _get_dma(cfg);
  DMA_Stream_Typedef *s = _get_stream(cfg);

  _clear_flags(d, cfg->stream);
  s->CR |= DMA_SxCR_EN;
}

/**
 * @brief Disable a DMA stream immediately.
 */
void dma_stop(const dma_config_t *cfg) {
  DMA_Stream_Typedef *s = _get_stream(cfg);
  s->CR &= ~DMA_SxCR_EN;
  while (s->CR & DMA_SxCR_EN)
    ;
}

/**
 * @brief Poll for transfer-complete flag.
 *
 * @return 1 if transfer complete, 0 otherwise.
 */
int dma_transfer_complete(const dma_config_t *cfg) {
  DMA_Typedef *d = _get_dma(cfg);
  return (*DMA_ISR_REG(d, cfg->stream) & DMA_ISR_TCIF(cfg->stream)) ? 1 : 0;
}

/**
 * @brief Clear all interrupt flags for the configured stream.
 */
void dma_clear_flags(const dma_config_t *cfg) {
  _clear_flags(_get_dma(cfg), cfg->stream);
}

/*---------------------------------------------------------------------------
 * Central DMA Interrupt Dispatchers
 * Each stream handler clears peripheral flags and routes to the HAL callback
 *system.
 *---------------------------------------------------------------------------*/

#define DMA_ISR_GEN(controller, stream, irqn)                                  \
  void controller##_Stream##stream##_IRQHandler(void) {                        \
    _clear_flags(controller, stream);                                          \
    hal_handle_interrupt(irqn);                                                \
  }

DMA_ISR_GEN(DMA1, 0, DMA1_Stream0_IRQn)
DMA_ISR_GEN(DMA1, 1, DMA1_Stream1_IRQn)
DMA_ISR_GEN(DMA1, 2, DMA1_Stream2_IRQn)
DMA_ISR_GEN(DMA1, 3, DMA1_Stream3_IRQn)
DMA_ISR_GEN(DMA1, 4, DMA1_Stream4_IRQn)
DMA_ISR_GEN(DMA1, 5, DMA1_Stream5_IRQn)
DMA_ISR_GEN(DMA1, 6, DMA1_Stream6_IRQn)
DMA_ISR_GEN(DMA1, 7, DMA1_Stream7_IRQn)

DMA_ISR_GEN(DMA2, 0, DMA2_Stream0_IRQn)
DMA_ISR_GEN(DMA2, 1, DMA2_Stream1_IRQn)
DMA_ISR_GEN(DMA2, 2, DMA2_Stream2_IRQn)
DMA_ISR_GEN(DMA2, 3, DMA2_Stream3_IRQn)
DMA_ISR_GEN(DMA2, 4, DMA2_Stream4_IRQn)
DMA_ISR_GEN(DMA2, 5, DMA2_Stream5_IRQn)
DMA_ISR_GEN(DMA2, 6, DMA2_Stream6_IRQn)
DMA_ISR_GEN(DMA2, 7, DMA2_Stream7_IRQn)

#endif /* _DMA_ENABLED */
