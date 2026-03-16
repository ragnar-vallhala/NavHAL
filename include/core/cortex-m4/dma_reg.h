/**
 * @file core/cortex-m4/dma_reg.h
 * @brief DMA peripheral register definitions for STM32F4.
 *
 * @details
 * Defines memory-mapped structures for DMA1 and DMA2 controllers,
 * including per-stream registers (CR, NDTR, PAR, M0AR, M1AR, FCR),
 * interrupt status/clear registers, and all relevant bit masks.
 *
 * Both DMA1 (base: 0x40026000) and DMA2 (base: 0x40026400) are covered.
 *
 * @note Only available when _DMA_ENABLED is defined.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_DMA_REG_H
#define CORTEX_M4_DMA_REG_H

#ifdef _DMA_ENABLED

#include "common/hal_types.h"
#include <stdint.h>

/*---------------------------------------------------------------------------
 * Per-stream register block
 *---------------------------------------------------------------------------*/

/**
 * @brief DMA stream register map.
 *
 * Each DMA controller has 8 streams; each stream has its own copy of these
 * registers starting at DMAx_BASE + 0x10 + stream_index * 0x18.
 */
typedef struct {
  __IO uint32_t CR;   /**< 0x00: Stream configuration register */
  __IO uint32_t NDTR; /**< 0x04: Number of data items to transfer */
  __IO uint32_t PAR;  /**< 0x08: Peripheral address */
  __IO uint32_t M0AR; /**< 0x0C: Memory 0 address */
  __IO uint32_t M1AR; /**< 0x10: Memory 1 address (double-buffer mode) */
  __IO uint32_t FCR;  /**< 0x14: FIFO control register */
} DMA_Stream_Typedef;

/*---------------------------------------------------------------------------
 * DMA controller register block
 *---------------------------------------------------------------------------*/

/**
 * @brief DMA controller register map.
 *
 * Holds interrupt status / clear registers and the 8 per-stream sub-blocks.
 */
typedef struct {
  __IO uint32_t
      LISR; /**< 0x00: Low  interrupt status  register (streams 0-3) */
  __IO uint32_t
      HISR; /**< 0x04: High interrupt status  register (streams 4-7) */
  __IO uint32_t LIFCR;          /**< 0x08: Low  interrupt flag clear register */
  __IO uint32_t HIFCR;          /**< 0x0C: High interrupt flag clear register */
  DMA_Stream_Typedef STREAM[8]; /**< 0x10..0xCC: Per-stream registers */
} DMA_Typedef;

/*---------------------------------------------------------------------------
 * Base addresses and instance pointers
 *---------------------------------------------------------------------------*/

#define DMA1_BASE_ADDR 0x40026000UL /**< DMA1 base address */
#define DMA2_BASE_ADDR 0x40026400UL /**< DMA2 base address */

#define DMA1 ((DMA_Typedef *)DMA1_BASE_ADDR) /**< DMA1 instance */
#define DMA2 ((DMA_Typedef *)DMA2_BASE_ADDR) /**< DMA2 instance */

/*---------------------------------------------------------------------------
 * RCC AHB1ENR clock-enable bits for DMA
 *---------------------------------------------------------------------------*/

#define RCC_AHB1ENR_DMA1EN (1U << 21) /**< DMA1 clock enable bit in AHB1ENR */
#define RCC_AHB1ENR_DMA2EN (1U << 22) /**< DMA2 clock enable bit in AHB1ENR */

/*---------------------------------------------------------------------------
 * DMA_SxCR – Stream configuration register bit definitions
 *---------------------------------------------------------------------------*/

#define DMA_SxCR_EN (1U << 0)     /**< Stream enable */
#define DMA_SxCR_DMEIE (1U << 1)  /**< Direct mode error interrupt enable */
#define DMA_SxCR_TEIE (1U << 2)   /**< Transfer error interrupt enable */
#define DMA_SxCR_HTIE (1U << 3)   /**< Half-transfer interrupt enable */
#define DMA_SxCR_TCIE (1U << 4)   /**< Transfer complete interrupt enable */
#define DMA_SxCR_PFCTRL (1U << 5) /**< Peripheral flow controller */

/** Direction field (bits [7:6]) */
#define DMA_SxCR_DIR_POS 6U
#define DMA_SxCR_DIR_MASK (0x3U << DMA_SxCR_DIR_POS)
#define DMA_SxCR_DIR_P2M                                                       \
  (0x0U << DMA_SxCR_DIR_POS) /**< Peripheral-to-memory                         \
                              */
#define DMA_SxCR_DIR_M2P                                                       \
  (0x1U << DMA_SxCR_DIR_POS)                        /**< Memory-to-peripheral  \
                                                     */
#define DMA_SxCR_DIR_M2M (0x2U << DMA_SxCR_DIR_POS) /**< Memory-to-memory */

#define DMA_SxCR_CIRC (1U << 8)  /**< Circular mode */
#define DMA_SxCR_PINC (1U << 9)  /**< Peripheral increment mode */
#define DMA_SxCR_MINC (1U << 10) /**< Memory increment mode */

/** Peripheral data size (bits [12:11]) */
#define DMA_SxCR_PSIZE_POS 11U
#define DMA_SxCR_PSIZE_MASK (0x3U << DMA_SxCR_PSIZE_POS)
#define DMA_SxCR_PSIZE_8 (0x0U << DMA_SxCR_PSIZE_POS)  /**< Byte */
#define DMA_SxCR_PSIZE_16 (0x1U << DMA_SxCR_PSIZE_POS) /**< Half-word */
#define DMA_SxCR_PSIZE_32 (0x2U << DMA_SxCR_PSIZE_POS) /**< Word */

/** Memory data size (bits [14:13]) */
#define DMA_SxCR_MSIZE_POS 13U
#define DMA_SxCR_MSIZE_MASK (0x3U << DMA_SxCR_MSIZE_POS)
#define DMA_SxCR_MSIZE_8 (0x0U << DMA_SxCR_MSIZE_POS)
#define DMA_SxCR_MSIZE_16 (0x1U << DMA_SxCR_MSIZE_POS)
#define DMA_SxCR_MSIZE_32 (0x2U << DMA_SxCR_MSIZE_POS)

/** Priority level (bits [17:16]) */
#define DMA_SxCR_PL_POS 16U
#define DMA_SxCR_PL_MASK (0x3U << DMA_SxCR_PL_POS)
#define DMA_SxCR_PL_LOW (0x0U << DMA_SxCR_PL_POS)
#define DMA_SxCR_PL_MEDIUM (0x1U << DMA_SxCR_PL_POS)
#define DMA_SxCR_PL_HIGH (0x2U << DMA_SxCR_PL_POS)
#define DMA_SxCR_PL_VHIGH (0x3U << DMA_SxCR_PL_POS)

/** Channel selection (bits [27:25]) */
#define DMA_SxCR_CHSEL_POS 25U
#define DMA_SxCR_CHSEL_MASK (0x7U << DMA_SxCR_CHSEL_POS)
#define DMA_SxCR_CHSEL(ch) (((ch) & 0x7U) << DMA_SxCR_CHSEL_POS)

/** Peripheral burst transfer configuration (bits [22:21]) */
#define DMA_SxCR_PBURST_POS 21U
#define DMA_SxCR_PBURST_MASK (0x3U << DMA_SxCR_PBURST_POS)
#define DMA_SxCR_PBURST_SINGLE (0x0U << DMA_SxCR_PBURST_POS)
#define DMA_SxCR_PBURST_INCR4 (0x1U << DMA_SxCR_PBURST_POS)
#define DMA_SxCR_PBURST_INCR8 (0x2U << DMA_SxCR_PBURST_POS)
#define DMA_SxCR_PBURST_INCR16 (0x3U << DMA_SxCR_PBURST_POS)

/** Memory burst transfer configuration (bits [24:23]) */
#define DMA_SxCR_MBURST_POS 23U
#define DMA_SxCR_MBURST_MASK (0x3U << DMA_SxCR_MBURST_POS)
#define DMA_SxCR_MBURST_SINGLE (0x0U << DMA_SxCR_MBURST_POS)
#define DMA_SxCR_MBURST_INCR4 (0x1U << DMA_SxCR_MBURST_POS)
#define DMA_SxCR_MBURST_INCR8 (0x2U << DMA_SxCR_MBURST_POS)
#define DMA_SxCR_MBURST_INCR16 (0x3U << DMA_SxCR_MBURST_POS)

/*---------------------------------------------------------------------------
 * DMA_SxFCR – FIFO control register
 *---------------------------------------------------------------------------*/

#define DMA_SxFCR_DMDIS (1U << 2) /**< Direct mode disable (enable FIFO) */
#define DMA_SxFCR_FTH_POS 0U
#define DMA_SxFCR_FTH_MASK (0x3U << DMA_SxFCR_FTH_POS)
#define DMA_SxFCR_FTH_1_4 (0x0U << DMA_SxFCR_FTH_POS)
#define DMA_SxFCR_FTH_1_2 (0x1U << DMA_SxFCR_FTH_POS)
#define DMA_SxFCR_FTH_3_4 (0x2U << DMA_SxFCR_FTH_POS)
#define DMA_SxFCR_FTH_FULL (0x3U << DMA_SxFCR_FTH_POS)

/*---------------------------------------------------------------------------
 * Interrupt status / flag-clear bit helpers (LISR/HISR/LIFCR/HIFCR)
 *
 * For stream n (0..7):
 *   - Streams 0,1   → LISR bits [5:0]  and [11:6]
 *   - Streams 2,3   → LISR bits [21:16] and [27:22]
 *   - Streams 4,5   → HISR bits [5:0]  and [11:6]
 *   - Streams 6,7   → HISR bits [21:16] and [27:22]
 *---------------------------------------------------------------------------*/

/** Bit-shift table (index = stream % 4) */
static const uint8_t _dma_isr_shift[4] = {0, 6, 16, 22};

/** Transfer-complete flag for stream n */
#define DMA_ISR_TCIF(n) (1U << (_dma_isr_shift[(n) % 4] + 5))
/** Half-transfer flag for stream n */
#define DMA_ISR_HTIF(n) (1U << (_dma_isr_shift[(n) % 4] + 4))
/** Transfer-error flag for stream n */
#define DMA_ISR_TEIF(n) (1U << (_dma_isr_shift[(n) % 4] + 3))
/** Direct-mode error flag for stream n */
#define DMA_ISR_DMEIF(n) (1U << (_dma_isr_shift[(n) % 4] + 2))
/** FIFO error flag for stream n */
#define DMA_ISR_FEIF(n) (1U << (_dma_isr_shift[(n) % 4] + 0))

/** Pointer to the correct ISR register (LISR/HISR) for stream n */
#define DMA_ISR_REG(dmax, n) (((n) < 4) ? &(dmax)->LISR : &(dmax)->HISR)
/** Pointer to the correct IFCR register (LIFCR/HIFCR) for stream n */
#define DMA_IFCR_REG(dmax, n) (((n) < 4) ? &(dmax)->LIFCR : &(dmax)->HIFCR)

#endif /* _DMA_ENABLED */

#endif /* CORTEX_M4_DMA_REG_H */
