/**
 * @file diskio.c
 * @brief SDIO implementation of the Disk I/O interface.
 */

#include "common/hal_diskio.h"
#include "core/cortex-m4/sdio.h"

static hal_disk_status_t disk_stat = HAL_DISK_STATUS_NOINIT;

hal_disk_status_t hal_disk_initialize(uint8_t pdrv) {
  if (pdrv != 0)
    return HAL_DISK_STATUS_NOINIT;

  /* We assume sdio_init() and PLL setup is handled by application for now,
     just as done in the sample. In a full OS-like setup, we'd do it here. */

  disk_stat &= ~HAL_DISK_STATUS_NOINIT;
  return disk_stat;
}

hal_disk_status_t hal_disk_status(uint8_t pdrv) {
  if (pdrv != 0)
    return HAL_DISK_STATUS_NODISK;
  return disk_stat;
}

hal_disk_result_t hal_disk_read(uint8_t pdrv, uint8_t *buff, uint32_t sector,
                                uint32_t count) {
  if (pdrv != 0 || !count)
    return HAL_DISK_RES_PARERR;
  if (disk_stat & HAL_DISK_STATUS_NOINIT)
    return HAL_DISK_RES_NOTRDY;

  while (count--) {
#ifdef _DMA_ENABLED
    if (sdio_read_block_dma(sector, buff) != HAL_SDIO_OK) {
#else
    if (sdio_read_block(sector, buff) != HAL_SDIO_OK) {
#endif
      return HAL_DISK_RES_ERROR;
    }
    sector++;
    buff += 512;
  }

  return HAL_DISK_RES_OK;
}

hal_disk_result_t hal_disk_write(uint8_t pdrv, const uint8_t *buff,
                                 uint32_t sector, uint32_t count) {
  if (pdrv != 0 || !count)
    return HAL_DISK_RES_PARERR;
  if (disk_stat & HAL_DISK_STATUS_NOINIT)
    return HAL_DISK_RES_NOTRDY;

  while (count--) {
#ifdef _DMA_ENABLED
    if (sdio_write_block_dma(sector, buff) != HAL_SDIO_OK) {
#else
    if (sdio_write_block(sector, buff) != HAL_SDIO_OK) {
#endif
      return HAL_DISK_RES_ERROR;
    }
    sector++;
    buff += 512;
  }

  return HAL_DISK_RES_OK;
}
hal_disk_result_t hal_disk_ioctl(uint8_t pdrv, uint8_t cmd, void *buff) {
  if (pdrv != 0)
    return HAL_DISK_RES_PARERR;
  if (disk_stat & HAL_DISK_STATUS_NOINIT)
    return HAL_DISK_RES_NOTRDY;

  switch (cmd) {
  case HAL_DISK_IO_SYNC:
    return HAL_DISK_RES_OK;
  case HAL_DISK_IO_GET_SECTOR_COUNT:
    *((uint32_t *)buff) = sdio_get_sector_count();
    return HAL_DISK_RES_OK;
  case HAL_DISK_IO_GET_SECTOR_SIZE:
    *((uint16_t *)buff) = 512;
    return HAL_DISK_RES_OK;
  case HAL_DISK_IO_GET_BLOCK_SIZE:
    *((uint32_t *)buff) = 1;
    return HAL_DISK_RES_OK;
  default:
    return HAL_DISK_RES_PARERR;
  }
}
