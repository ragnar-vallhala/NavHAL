#include "core/cortex-m4/sdio.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/interrupt.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/uart.h"
#include <stdint.h>

/**
 * @file sdio.c
 * @brief SDIO driver implementation for STM32F4.
 */

static uint32_t sd_rca = 0;
static uint8_t card_is_sdhc = 0;
static uint8_t desired_bus_width = 0;
static hal_sdio_callback_t sd_callback = 0;
static volatile uint8_t sd_busy = 0;
static volatile uint8_t dma_done = 0;
static volatile uint8_t sdio_done = 0;
static volatile uint8_t is_multi_block = 0;
static hal_sdio_error_t sd_last_error = HAL_SDIO_OK;

/* ------------------------------------------------------------- */
/* INIT */
/* ------------------------------------------------------------- */

static uint32_t get_sdioclk(void) {
  uint8_t sws = ((RCC->CFGR) >> 2) & 0x3; // RCC_CFGR_SWS_BIT
  if (sws == 2) {                         // PLL
    uint32_t pll_m = (RCC->PLLCFGR >> 0) & 0x3F;
    uint32_t pll_n = (RCC->PLLCFGR >> 6) & 0x1FF;
    uint32_t pll_q = (RCC->PLLCFGR >> 24) & 0xF;
    uint32_t pll_src = (RCC->PLLCFGR >> 22) & 0x1;
    uint32_t vco_in = pll_src ? 8000000 : 16000000;
    return (vco_in / pll_m) * pll_n / pll_q;
  }
  return hal_clock_get_sysclk();
}

hal_sdio_error_t sdio_init(const hal_sdio_config_t *config) {
  if (!config)
    return HAL_SDIO_ERROR;

  hal_gpio_set_alternate_function(GPIO_PC08, GPIO_AF12);
  hal_gpio_set_alternate_function(GPIO_PC09, GPIO_AF12);
  hal_gpio_set_alternate_function(GPIO_PC10, GPIO_AF12);
  hal_gpio_set_alternate_function(GPIO_PC11, GPIO_AF12);
  hal_gpio_set_alternate_function(GPIO_PC12, GPIO_AF12);
  hal_gpio_set_alternate_function(GPIO_PD02, GPIO_AF12);

  hal_gpio_pin pins[] = {GPIO_PC08, GPIO_PC09, GPIO_PC10,
                         GPIO_PC11, GPIO_PC12, GPIO_PD02};

  for (int i = 0; i < 6; i++) {
    hal_gpio_set_output_speed(pins[i], GPIO_VERY_HIGH_SPEED);
    hal_gpio_set_output_type(pins[i], GPIO_PUSH_PULL);
    hal_gpio_setmode(pins[i], GPIO_AF, GPIO_PULLUP);
  }

  RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;

  SDIO->POWER = SDIO_POWER_PWRCTRL_ON;

  desired_bus_width = config->bus_width;

  uint32_t sdioclk = get_sdioclk();
  uint32_t div = (sdioclk / 400000);
  if (div >= 2)
    div -= 2;
  else
    div = 0;
  if (div > 255)
    div = 255;

  uint32_t clkcr = div & 0xFF;

  /* Switch to 4-bit mode if requested (handled in sdio_card_init) */
  /* hardware flow control is essential for DMA write */
  clkcr |= SDIO_CLKCR_HWFC_EN | SDIO_CLKCR_CLKEN;

  SDIO->CLKCR = clkcr;

  /* Enable SDIO Interrupts in NVIC */
  hal_enable_interrupt(SDIO_IRQn);
  hal_set_interrupt_priority(SDIO_IRQn, 5);

#ifdef _DMA_ENABLED
  hal_enable_interrupt(DMA2_Stream3_IRQn);
  hal_enable_interrupt(DMA2_Stream6_IRQn);
  hal_set_interrupt_priority(DMA2_Stream3_IRQn, 5);
  hal_set_interrupt_priority(DMA2_Stream6_IRQn, 5);
#endif

  return HAL_SDIO_OK;
}

void sdio_set_callback(hal_sdio_callback_t callback) { sd_callback = callback; }

/* ------------------------------------------------------------- */
/* COMMAND HANDLING */
/* ------------------------------------------------------------- */

hal_sdio_error_t sdio_wait_flag(uint32_t flag, uint32_t timeout) {
  while (!(SDIO->STA & flag) && timeout)
    timeout--;

  return timeout ? HAL_SDIO_OK : HAL_SDIO_TIMEOUT;
}

hal_sdio_error_t sdio_send_command(uint8_t cmd, uint32_t arg, uint32_t resp) {
  SDIO->ICR = 0xFFFFFFFF;

  SDIO->ARG = arg;

  uint32_t reg = (cmd & SDIO_CMD_CMDINDEX_Msk) | SDIO_CMD_CPSMEN;

  if (resp == 1 || resp == 2)
    reg |= SDIO_CMD_WAITRESP_SHORT;
  else if (resp == 3)
    reg |= SDIO_CMD_WAITRESP_LONG;

  SDIO->CMD = reg;

  if (resp == 0)
    return sdio_wait_flag(SDIO_STA_CMDSENT, 100000);

  uint32_t timeout = 100000;
  while (timeout--) {
    uint32_t sta = SDIO->STA;

    if (sta & SDIO_STA_CTIMEOUT) {
      return HAL_SDIO_TIMEOUT;
    }

    if (sta & SDIO_STA_CCRCFAIL) {
      if (resp == 1 || resp == 3) // CRC-protected responses
      {
        return HAL_SDIO_CRC_FAIL;
      }
      // R3 (resp == 2) has no CRC, but hardware sets CCRCFAIL. We treat as
      // success.
      return HAL_SDIO_OK;
    }

    if (sta & SDIO_STA_CMDREND)
      return HAL_SDIO_OK;
  }

  return HAL_SDIO_TIMEOUT;
}

uint32_t sdio_get_response(uint8_t reg) {
  switch (reg) {
  case 1:
    return SDIO->RESP1;
  case 2:
    return SDIO->RESP2;
  case 3:
    return SDIO->RESP3;
  case 4:
    return SDIO->RESP4;
  default:
    return 0;
  }
}

/* ------------------------------------------------------------- */
/* CARD READY */
/* ------------------------------------------------------------- */

static hal_sdio_error_t sdio_wait_card_ready(void) {
  uint32_t timeout = 500;

  while (timeout--) {
    if (sdio_send_command(SD_CMD_SEND_STATUS, sd_rca, 1) == HAL_SDIO_OK) {
      uint32_t status = sdio_get_response(1);

      uint32_t state = (status >> 9) & 0xF;

      if (state == 4) // TRANSFER STATE
        return HAL_SDIO_OK;
    }

    delay_ms(1);
  }

  return HAL_SDIO_TIMEOUT;
}

/* ------------------------------------------------------------- */
/* CARD INIT */
/* ------------------------------------------------------------- */

hal_sdio_error_t sdio_card_init(void) {
  static uint8_t initialized = 0;
  if (initialized) {
    return HAL_SDIO_OK; /* Already initialized, skip handshake */
  }

  hal_sdio_error_t err;
  uint32_t resp;

  err = sdio_send_command(SD_CMD_GO_IDLE_STATE, 0, 0);
  if (err)
    return err;

  sdio_send_command(SD_CMD_HS_SEND_EXT_CSD, 0x1AA, 1);

  uint32_t timeout = 1000;

  while (timeout--) {
    sdio_send_command(SD_CMD_APP_CMD, 0, 1);

    err = sdio_send_command(SD_ACMD_SD_SEND_OP_COND, 0x40FF8000, 2);
    if (err)
      return err;

    resp = sdio_get_response(1);

    if (resp & (1 << 31)) {
      if (resp & (1 << 30))
        card_is_sdhc = 1;
      break;
    }

    delay_ms(1);
  }

  if (!timeout)
    return HAL_SDIO_TIMEOUT;

  sdio_send_command(SD_CMD_ALL_SEND_CID, 0, 3);

  sdio_send_command(SD_CMD_SEND_REL_ADDR, 0, 1);
  sd_rca = sdio_get_response(1) & 0xFFFF0000;

  sdio_send_command(SD_CMD_SELECT_DESELECT_CARD, sd_rca, 1);
  if (!card_is_sdhc)
    sdio_send_command(SD_CMD_SET_BLOCKLEN, 512, 1);

  /* Switch to 4-bit mode if requested */
  if (desired_bus_width == 1) {
    if (sdio_send_command(SD_CMD_APP_CMD, sd_rca, 1) == HAL_SDIO_OK) {
      if (sdio_send_command(SD_ACMD_SET_BUS_WIDTH, 2, 1) == HAL_SDIO_OK) {
        SDIO->CLKCR =
            (SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS_Msk) | SDIO_CLKCR_WIDBUS_4B;
      }
    }
  }

  /* Set clock to 21 MHz (SDIOCLK=84MHz / (2+2) = 21MHz) */
  /* DIV=2 for stable high-speed, keep WIDBUS, enable HWFC */
  SDIO->CLKCR = (SDIO->CLKCR & ~(SDIO_CLKCR_CLKDIV | SDIO_CLKCR_HWFC_EN |
                                 SDIO_CLKCR_PWRSAV)) |
                2 | SDIO_CLKCR_HWFC_EN | SDIO_CLKCR_CLKEN;

  initialized = 1;
  return HAL_SDIO_OK;
}

/* ------------------------------------------------------------- */
/* READ BLOCK */
/* ------------------------------------------------------------- */

hal_sdio_error_t sdio_read_block(uint32_t addr, uint8_t *buf) {
  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  /* Configure DPSM Parameters: DTEN must be enabled for Read */
  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;
  SDIO->DCTRL =
      (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;

  if (sdio_send_command(SD_CMD_READ_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    return HAL_SDIO_ERROR;
  }

  uint32_t *p = (uint32_t *)buf;
  int words = 128;
  uint32_t timeout = 5000000;

  __asm volatile("cpsid i" : : : "memory");
  while (words > 0 && timeout--) {
    uint32_t sta = SDIO->STA;

    if (sta & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      __asm volatile("cpsie i" : : : "memory");
      return HAL_SDIO_ERROR;
    }

    if (sta & SDIO_STA_RXFIFOHF) {
      for (int i = 0; i < 8 && words > 0; i++) {
        *p++ = SDIO->FIFO;
        words--;
      }
      timeout = 5000000;
    } else if (sta & SDIO_STA_RXDAVL) {
      *p++ = SDIO->FIFO;
      words--;
      timeout = 5000000;
    }
  }
  __asm volatile("cpsie i" : : : "memory");

  if (words > 0) {
    SDIO->DCTRL = 0;
    return HAL_SDIO_TIMEOUT;
  }

  /* Wait for DBCKEND (Confirm block CRC passed) */
  timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DBCKEND) && timeout--) {
    uint32_t sta = SDIO->STA;
    if (sta & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0) {
    SDIO->DCTRL = 0;
    uart2_write_string("SDIO Read DBCKEND Timeout\n\r");
    return HAL_SDIO_TIMEOUT;
  }

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->DCTRL = 0;
  return HAL_SDIO_OK;
}

/* ------------------------------------------------------------- */
/* WRITE BLOCK */
/* ------------------------------------------------------------- */
hal_sdio_error_t sdio_write_block(uint32_t addr, const uint8_t *buf) {
  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  /* Configure DPSM Parameters */
  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;

  /* ST Recommended: Enable DTEN BEFORE sending the command */
  SDIO->DCTRL = (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DTEN;

  if (sdio_send_command(SD_CMD_WRITE_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    return HAL_SDIO_ERROR;
  }

  const uint32_t *p = (const uint32_t *)buf;
  int words = 128;
  uint32_t timeout = 5000000;

  __asm volatile("cpsid i" : : : "memory");
  while (words > 0 && timeout--) {
    uint32_t sta = SDIO->STA;

    if (sta & (SDIO_STA_TXUNDERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      __asm volatile("cpsie i" : : : "memory");
      return HAL_SDIO_ERROR;
    }

    /* Polling for FIFO space. FIFO deep = 32 words. half-full = 16 words. */
    if (sta & SDIO_STA_TXFIFOHE) {
      for (int i = 0; i < 8 && words > 0; i++) {
        SDIO->FIFO = *p++;
        words--;
      }
      timeout = 5000000;
    }
  }
  __asm volatile("cpsie i" : : : "memory");

  if (words > 0) {
    SDIO->DCTRL = 0;
    return HAL_SDIO_TIMEOUT;
  }

  /* Wait for DBCKEND (Confirm card received block with good CRC) */
  timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DBCKEND) && timeout--) {
    uint32_t sta = SDIO->STA;
    if (sta & (SDIO_STA_TXUNDERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0) {
    SDIO->DCTRL = 0;
    return HAL_SDIO_TIMEOUT;
  }

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->DCTRL = 0;

  return sdio_wait_card_ready();
}

uint32_t sdio_get_sector_count(void) {
  uint32_t csd[4];

  /* Send CMD9 (SEND_CSD) to get card capacity. Long response (R2) */
  if (sdio_send_command(9, sd_rca, 3) != HAL_SDIO_OK) {
    return 0;
  }

  /* RESP1 contains bits [127:96], RESP2 [95:64], RESP3 [63:32], RESP4 [31:0]
   */
  csd[0] = SDIO->RESP1;
  csd[1] = SDIO->RESP2;
  csd[2] = SDIO->RESP3;
  csd[3] = SDIO->RESP4;

  /* CSD Structure depends on CSD_STRUCTURE field [127:126] */
  uint8_t csd_struct = (csd[0] >> 30) & 0x3;

  if (csd_struct == 1) { /* CSD Version 2.0 (High Capacity/Extended Capacity) */
    /* C_SIZE is at [69:48] of CSD structure */
    /* RESP2 bits [5:0] and RESP3 bits [31:16] */
    uint32_t c_size =
        ((csd[1] & 0x0000003F) << 16) | ((csd[2] & 0xFFFF0000) >> 16);
    return (c_size + 1) * 1024; /* Capacity in sectors (512 bytes each) */
  } else if (csd_struct == 0) { /* CSD Version 1.0 (Standard Capacity) */
    /* C_SIZE [73:62], C_SIZE_MULT [49:47], READ_BL_LEN [83:80] */
    uint32_t c_size =
        ((csd[1] & 0x000003FF) << 2) | ((csd[2] & 0xC0000000) >> 30);
    uint8_t c_size_mult = (csd[2] >> 15) & 0x7;
    uint8_t read_bl_len = (csd[1] >> 16) & 0xF;
    uint32_t mult = 1 << (c_size_mult + 2);
    uint32_t block_len = 1 << read_bl_len;
    return (c_size + 1) * mult * (block_len / 512);
  }

  return 0;
}

#ifdef _DMA_ENABLED
#include "core/cortex-m4/dma.h"
#include "core/cortex-m4/uart.h"

static dma_config_t dma2_stream3_cfg;
static dma_config_t dma2_stream6_cfg;

static void _sdio_dma_rx_irq_handler(void);
static void _sdio_dma_tx_irq_handler(void);

hal_sdio_error_t sdio_read_block_async(uint32_t addr, uint8_t *buf) {
  if (sd_busy)
    return HAL_SDIO_BUSY;

  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  dma2_stream3_cfg = (dma_config_t){
      .controller = DMA_CONTROLLER_2,
      .stream = 3,
      .channel = 4,
      .direction = DMA_DIR_P2M,
      .src_addr = (uint32_t)&SDIO->FIFO,
      .dst_addr = (uint32_t)buf,
      .data_count = 512 / 4,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = DMA_DATA_WIDTH_32,
      .priority = DMA_PRIORITY_VERY_HIGH,
      .circular = 0,
      .pfctrl = 1,
      .fifo_mode = 1,
      .fifo_threshold = DMA_FIFO_THRESHOLD_FULL,
      .mburst = DMA_BURST_INCR4,
      .pburst = DMA_BURST_INCR4,
  };

  dma_init((const dma_config_t *)&dma2_stream3_cfg);
  hal_interrupt_attach_callback(DMA2_Stream3_IRQn, _sdio_dma_rx_irq_handler);

  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;
  SDIO->DCTRL = (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DTDIR |
                SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  dma_start((const dma_config_t *)&dma2_stream3_cfg);

  if (sdio_send_command(SD_CMD_READ_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    dma_stop((const dma_config_t *)&dma2_stream3_cfg);
    return HAL_SDIO_ERROR;
  }

  sd_busy = 1;
  dma_done = 0;
  sdio_done = 0;
  is_multi_block = 0;
  sd_last_error = HAL_SDIO_OK;

  /* Enable SDIO interrupts: DATAEND, DBCKEND, and error flags */
  SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DBCKENDIE |
                SDIO_MASK_DCRCFAILIE | SDIO_MASK_DTIMEOUTIE |
                SDIO_MASK_RXOVERRIE;

  return HAL_SDIO_PENDING;
}

hal_sdio_error_t sdio_write_block_async(uint32_t addr, const uint8_t *buf) {
  if (sd_busy)
    return HAL_SDIO_BUSY;

  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  dma2_stream6_cfg = (dma_config_t){
      .controller = DMA_CONTROLLER_2,
      .stream = 6,
      .channel = 4,
      .direction = DMA_DIR_M2P,
      .src_addr = (uint32_t)buf,
      .dst_addr = (uint32_t)&SDIO->FIFO,
      .data_count = 512 / 4,
      .src_inc = 1,
      .dst_inc = 0,
      .data_width = DMA_DATA_WIDTH_32,
      .priority = DMA_PRIORITY_VERY_HIGH,
      .circular = 0,
      .pfctrl = 1,
      .fifo_mode = 1,
      .fifo_threshold = DMA_FIFO_THRESHOLD_FULL,
      .mburst = DMA_BURST_INCR4,
      .pburst = DMA_BURST_INCR4,
  };

  dma_init((const dma_config_t *)&dma2_stream6_cfg);
  hal_interrupt_attach_callback(DMA2_Stream6_IRQn, _sdio_dma_tx_irq_handler);

  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;
  SDIO->DCTRL =
      (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  if (sdio_send_command(SD_CMD_WRITE_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    dma_stop((const dma_config_t *)&dma2_stream6_cfg);
    return HAL_SDIO_ERROR;
  }
  dma_start((const dma_config_t *)&dma2_stream6_cfg);

  sd_busy = 1;
  dma_done = 0;
  sdio_done = 0;
  sd_last_error = HAL_SDIO_OK;

  /* Enable SDIO interrupts: DATAEND, DBCKEND, and error flags */
  SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DBCKENDIE |
                SDIO_MASK_DCRCFAILIE | SDIO_MASK_DTIMEOUTIE |
                SDIO_MASK_TXUNDERRIE;

  return HAL_SDIO_PENDING;
}

hal_sdio_error_t sdio_read_blocks_async(uint32_t addr, uint8_t *buf,
                                        uint32_t count) {
  if (sd_busy)
    return HAL_SDIO_BUSY;

  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  dma2_stream3_cfg = (dma_config_t){
      .controller = DMA_CONTROLLER_2,
      .stream = 3,
      .channel = 4,
      .direction = DMA_DIR_P2M,
      .src_addr = (uint32_t)&SDIO->FIFO,
      .dst_addr = (uint32_t)buf,
      .data_count = (512 / 4) * count,
      .src_inc = 0,
      .dst_inc = 1,
      .data_width = DMA_DATA_WIDTH_32,
      .priority = DMA_PRIORITY_VERY_HIGH,
      .circular = 0,
      .pfctrl = 1,
      .fifo_mode = 1,
      .fifo_threshold = DMA_FIFO_THRESHOLD_FULL,
      .mburst = DMA_BURST_INCR4,
      .pburst = DMA_BURST_INCR4,
  };

  dma_init((const dma_config_t *)&dma2_stream3_cfg);
  hal_interrupt_attach_callback(DMA2_Stream3_IRQn, _sdio_dma_rx_irq_handler);
  SDIO->DCTRL = 0;
  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512 * count;

  if (sdio_send_command(SD_CMD_READ_MULT_BLOCK, addr, 1)) {
    dma_stop((const dma_config_t *)&dma2_stream3_cfg);
#ifdef _DMA_ENABLED
    uart2_write_string("Read Multi CMD18 failed\r\n");
#endif
    return HAL_SDIO_ERROR;
  }

  /* Enable DPSM AFTER command per spec */
  SDIO->DCTRL = (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DTDIR |
                SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  dma_start((const dma_config_t *)&dma2_stream3_cfg);

  sd_busy = 1;
  dma_done = 0;
  sdio_done = 0;
  is_multi_block = 1;
  sd_last_error = HAL_SDIO_OK;

  /* Enable SDIO interrupts: DATAEND, DBCKEND, and error flags */
  SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DBCKENDIE |
                SDIO_MASK_DCRCFAILIE | SDIO_MASK_DTIMEOUTIE |
                SDIO_MASK_RXOVERRIE;

  return HAL_SDIO_PENDING;
}

hal_sdio_error_t sdio_write_blocks_async(uint32_t addr, const uint8_t *buf,
                                         uint32_t count) {
  if (sd_busy)
    return HAL_SDIO_BUSY;

  if (!card_is_sdhc)
    addr *= 512;

  SDIO->ICR = 0xFFFFFFFF;

  dma2_stream6_cfg = (dma_config_t){
      .controller = DMA_CONTROLLER_2,
      .stream = 6,
      .channel = 4,
      .direction = DMA_DIR_M2P,
      .src_addr = (uint32_t)buf,
      .dst_addr = (uint32_t)&SDIO->FIFO,
      .data_count = (512 / 4) * count,
      .src_inc = 1,
      .dst_inc = 0,
      .data_width = DMA_DATA_WIDTH_32,
      .priority = DMA_PRIORITY_VERY_HIGH,
      .circular = 0,
      .pfctrl = 1,
      .fifo_mode = 1,
      .fifo_threshold = DMA_FIFO_THRESHOLD_FULL,
      .mburst = DMA_BURST_INCR4,
      .pburst = DMA_BURST_INCR4,
  };

  dma_init((const dma_config_t *)&dma2_stream6_cfg);
  hal_interrupt_attach_callback(DMA2_Stream6_IRQn, _sdio_dma_tx_irq_handler);

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512 * count;

  if (sdio_send_command(SD_CMD_WRITE_MULT_BLOCK, addr, 1)) {
    dma_stop((const dma_config_t *)&dma2_stream6_cfg);
#ifdef _DMA_ENABLED
    uart2_write_string("Write Multi CMD25 failed\r\n");
#endif
    return HAL_SDIO_ERROR;
  }

  /* FIX: Start DMA FIRST so it pre-fills the SDIO FIFO */
  dma_start((const dma_config_t *)&dma2_stream6_cfg);

  /* FIX: Enable DPSM AFTER DMA is running to avoid TXUNDERRUN */
  SDIO->DCTRL =
      (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  sd_busy = 1;
  dma_done = 0;
  sdio_done = 0;
  is_multi_block = 1;
  sd_last_error = HAL_SDIO_OK;

  /* Enable SDIO interrupts: DATAEND and error flags */
  SDIO->MASK |= SDIO_MASK_DATAENDIE | SDIO_MASK_DCRCFAILIE |
                SDIO_MASK_DTIMEOUTIE | SDIO_MASK_TXUNDERRIE;

  return HAL_SDIO_PENDING;
}

void SDIO_IRQHandler(void) {
  uint32_t sta = SDIO->STA;
  hal_sdio_error_t err = HAL_SDIO_OK;

  if (sta & SDIO_STA_DCRCFAIL)
    err = HAL_SDIO_CRC_FAIL;
  else if (sta & SDIO_STA_DTIMEOUT)
    err = HAL_SDIO_TIMEOUT;
  else if (sta & SDIO_STA_RXOVERR)
    err = HAL_SDIO_RX_OVERRUN;
  else if (sta & SDIO_STA_TXUNDERR)
    err = HAL_SDIO_TX_UNDERRUN;

  /* FIX: Use DATAEND instead of DBCKEND to support multi-block */
  if (err != HAL_SDIO_OK || (sta & SDIO_STA_DATAEND)) {
    SDIO->MASK &=
        ~(SDIO_MASK_DATAENDIE | SDIO_MASK_DBCKENDIE | SDIO_MASK_DCRCFAILIE |
          SDIO_MASK_DTIMEOUTIE | SDIO_MASK_RXOVERRIE | SDIO_MASK_TXUNDERRIE);
    SDIO->ICR = 0xFFFFFFFF;
    SDIO->DCTRL = 0;
    sd_last_error = err;
    sdio_done = 1;

    /* Coordination with DMA TC */
    if (dma_done || err != HAL_SDIO_OK) {
      sd_busy = 0;
      if (sd_callback) {
        sd_callback(err);
      }
    }
  }
}

static void _sdio_dma_rx_irq_handler(void) {
  dma_clear_flags((const dma_config_t *)&dma2_stream3_cfg);
  dma_done = 1;
  if (sdio_done || sd_last_error != HAL_SDIO_OK) {
    sd_busy = 0;
    if (sd_callback) {
      sd_callback(sd_last_error);
    }
  }
}

static void _sdio_dma_tx_irq_handler(void) {
  dma_clear_flags((const dma_config_t *)&dma2_stream6_cfg);
  dma_done = 1;
  if (sdio_done || sd_last_error != HAL_SDIO_OK) {
    sd_busy = 0;
    if (sd_callback) {
      sd_callback(sd_last_error);
    }
  }
}

hal_sdio_error_t sdio_wait_sync(hal_sdio_error_t result) {
  if (result != HAL_SDIO_PENDING)
    return result;

  uint32_t start = hal_get_millis();
  uint32_t timeout_ms = 10000;
  if (is_multi_block)
    timeout_ms = 1000;
  else
    timeout_ms = 200;
  while (sd_busy) {
    if ((uint32_t)(hal_get_millis() - start) >= timeout_ms)
      break;
    __asm volatile("wfi");
    // fallback safety
    __asm volatile("nop");
  }

  if (hal_get_millis() - start >= timeout_ms) {
    return HAL_SDIO_TIMEOUT;
  }

  if (sd_last_error == HAL_SDIO_OK && is_multi_block) {
    if (sdio_send_command(SD_CMD_STOP_TRANSMISSION, sd_rca, 1) != HAL_SDIO_OK) {
      return HAL_SDIO_ERROR;
    }

    uint32_t start = hal_get_millis();
    while (sdio_wait_card_ready() != HAL_SDIO_OK) {
      if ((uint32_t)(hal_get_millis() - start) >= 500) {
        return HAL_SDIO_TIMEOUT;
      }
    }

    is_multi_block = 0;
  }

  return sd_last_error;
}
#endif
