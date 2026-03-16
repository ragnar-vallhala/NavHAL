#include "core/cortex-m4/sdio.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/gpio.h"
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

  return HAL_SDIO_OK;
}

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

  /* Set clock to a practical speed after verification (approx 4.8 MHz) */
  /* DIV=8, keep WIDBUS, disable HWFC for now */
  SDIO->CLKCR = (SDIO->CLKCR & ~(SDIO_CLKCR_CLKDIV | SDIO_CLKCR_HWFC_EN |
                                 SDIO_CLKCR_PWRSAV)) |
                8 | SDIO_CLKCR_CLKEN;

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

  if (timeout == 0xFFFFFFFF) {
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

  if (timeout == 0xFFFFFFFF) {
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

hal_sdio_error_t sdio_read_block_dma(uint32_t addr, uint8_t *buf) {
  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  dma_config_t dma_read_cfg = {
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

  dma_init(&dma_read_cfg);

  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;
  SDIO->DCTRL = (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DTDIR |
                SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  dma_start(&dma_read_cfg);

  if (sdio_send_command(SD_CMD_READ_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_read_cfg);
    return HAL_SDIO_ERROR;
  }

  uint32_t timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DATAEND) && timeout--) {
    if (SDIO->STA &
        (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      dma_stop(&dma_read_cfg);
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0xFFFFFFFF) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_read_cfg);
    return HAL_SDIO_TIMEOUT;
  }

  timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DBCKEND) && timeout--) {
    uint32_t sta = SDIO->STA;
    if (sta & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0xFFFFFFFF) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_read_cfg);
    return HAL_SDIO_TIMEOUT;
  }

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->DCTRL = 0;

  while (!dma_transfer_complete(&dma_read_cfg))
    ;
  dma_clear_flags(&dma_read_cfg);

  return HAL_SDIO_OK;
}

hal_sdio_error_t sdio_write_block_dma(uint32_t addr, const uint8_t *buf) {
  if (!card_is_sdhc)
    addr *= 512;

  if (sdio_wait_card_ready())
    return HAL_SDIO_TIMEOUT;

  SDIO->ICR = 0xFFFFFFFF;

  dma_config_t dma_write_cfg = {
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

  dma_init(&dma_write_cfg);

  SDIO->DTIMER = 0xFFFFFFFF;
  SDIO->DLEN = 512;
  SDIO->DCTRL =
      (9 << SDIO_DCTRL_DBLOCKSIZE_Pos) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

  if (sdio_send_command(SD_CMD_WRITE_SINGLE_BLOCK, addr, 1)) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_write_cfg);
    return HAL_SDIO_ERROR;
  }
  dma_start(&dma_write_cfg);
  uint32_t timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DATAEND) && timeout--) {
    if (SDIO->STA &
        (SDIO_STA_TXUNDERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {

      SDIO->DCTRL = 0;
      dma_stop(&dma_write_cfg);
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0xFFFFFFFF) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_write_cfg);
    return HAL_SDIO_TIMEOUT;
  }

  timeout = 5000000;
  while (!(SDIO->STA & SDIO_STA_DBCKEND) && timeout--) {
    if (SDIO->STA &
        (SDIO_STA_TXUNDERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)) {
      SDIO->DCTRL = 0;
      dma_stop(&dma_write_cfg);
      return HAL_SDIO_ERROR;
    }
  }

  if (timeout == 0xFFFFFFFF) {
    SDIO->DCTRL = 0;
    dma_stop(&dma_write_cfg);
    return HAL_SDIO_TIMEOUT;
  }

  SDIO->ICR = 0xFFFFFFFF;
  SDIO->DCTRL = 0;

  while (!dma_transfer_complete(&dma_write_cfg))
    ;
  dma_clear_flags(&dma_write_cfg);

  return sdio_wait_card_ready();
}
#endif