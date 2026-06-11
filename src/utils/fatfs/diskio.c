/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file diskio.c
 * @brief Glue code between FatFS and NavHAL hal_diskio.
 */

#include "diskio.h"
#include "common/hal_diskio.h"

DSTATUS disk_status(uint8_t pdrv) {
  hal_disk_status_t status = hal_disk_status(pdrv);
  DSTATUS dstat = 0;

  if (status & HAL_DISK_STATUS_NOINIT)
    dstat |= STA_NOINIT;
  if (status & HAL_DISK_STATUS_NODISK)
    dstat |= STA_NODISK;
  if (status & HAL_DISK_STATUS_PROTECT)
    dstat |= STA_PROTECT;

  return dstat;
}

DSTATUS disk_initialize(uint8_t pdrv) {
  hal_disk_status_t status = hal_disk_initialize(pdrv);
  DSTATUS dstat = 0;

  if (status & HAL_DISK_STATUS_NOINIT)
    dstat |= STA_NOINIT;
  if (status & HAL_DISK_STATUS_NODISK)
    dstat |= STA_NODISK;
  if (status & HAL_DISK_STATUS_PROTECT)
    dstat |= STA_PROTECT;

  return dstat;
}

DRESULT disk_read(uint8_t pdrv, uint8_t *buff, uint32_t sector,
                  uint32_t count) {
  hal_disk_result_t res = hal_disk_read(pdrv, buff, sector, count);

  switch (res) {
  case HAL_DISK_RES_OK:
    return RES_OK;
  case HAL_DISK_RES_ERROR:
    return RES_ERROR;
  case HAL_DISK_RES_WRPRT:
    return RES_WRPRT;
  case HAL_DISK_RES_NOTRDY:
    return RES_NOTRDY;
  case HAL_DISK_RES_PARERR:
    return RES_PARERR;
  default:
    return RES_ERROR;
  }
}

DRESULT disk_write(uint8_t pdrv, const uint8_t *buff, uint32_t sector,
                   uint32_t count) {
  hal_disk_result_t res = hal_disk_write(pdrv, buff, sector, count);

  switch (res) {
  case HAL_DISK_RES_OK:
    return RES_OK;
  case HAL_DISK_RES_ERROR:
    return RES_ERROR;
  case HAL_DISK_RES_WRPRT:
    return RES_WRPRT;
  case HAL_DISK_RES_NOTRDY:
    return RES_NOTRDY;
  case HAL_DISK_RES_PARERR:
    return RES_PARERR;
  default:
    return RES_ERROR;
  }
}

DRESULT disk_ioctl(uint8_t pdrv, uint8_t cmd, void *buff) {
  hal_disk_result_t res;
  uint8_t hal_cmd;

  switch (cmd) {
  case CTRL_SYNC:
    hal_cmd = HAL_DISK_IO_SYNC;
    break;
  case GET_SECTOR_COUNT:
    hal_cmd = HAL_DISK_IO_GET_SECTOR_COUNT;
    break;
  case GET_SECTOR_SIZE:
    hal_cmd = HAL_DISK_IO_GET_SECTOR_SIZE;
    break;
  case GET_BLOCK_SIZE:
    hal_cmd = HAL_DISK_IO_GET_BLOCK_SIZE;
    break;
  default:
    return RES_PARERR;
  }

  res = hal_disk_ioctl(pdrv, hal_cmd, buff);
  return (res == HAL_DISK_RES_OK) ? RES_OK : RES_ERROR;
}

/* FatFS requires a get_fattime function if not provided by user */
__attribute__((weak)) uint32_t get_fattime(void) {
  /* Returns a fixed time for now: 2024-01-01 00:00:00 */
  return ((uint32_t)(2024 - 1980) << 25) | ((uint32_t)1 << 21) |
         ((uint32_t)1 << 16);
}
