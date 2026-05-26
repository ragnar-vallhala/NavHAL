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
 * @file hal_diskio.h
 * @brief Common Hardware Abstraction Layer (HAL) for block devices.
 *
 * Defines the standard interface for block-level storage (SD cards, Flash,
 * etc.) required by filesystem libraries like FatFs.
 */

#ifndef HAL_DISKIO_H
#define HAL_DISKIO_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Disk Status bitmask
 */
typedef uint8_t hal_disk_status_t;
#define HAL_DISK_STATUS_OK 0x00
#define HAL_DISK_STATUS_NOINIT 0x01
#define HAL_DISK_STATUS_NODISK 0x02
#define HAL_DISK_STATUS_PROTECT 0x04

/**
 * @brief Disk Result codes
 */
typedef enum {
  HAL_DISK_RES_OK = 0, /**< 0: Successful */
  HAL_DISK_RES_ERROR,  /**< 1: R/W Error */
  HAL_DISK_RES_WRPRT,  /**< 2: Write Protected */
  HAL_DISK_RES_NOTRDY, /**< 3: Not Ready */
  HAL_DISK_RES_PARERR  /**< 4: Invalid Parameter */
} hal_disk_result_t;

/**
 * @brief Disk I/O Interface functions
 */

hal_disk_status_t hal_disk_initialize(uint8_t pdrv);
hal_disk_status_t hal_disk_status(uint8_t pdrv);

hal_disk_result_t
hal_disk_read(uint8_t pdrv,    /**< Physical drive number */
              uint8_t *buff,   /**< Data buffer to store read data */
              uint32_t sector, /**< Sector address (LBA) */
              uint32_t count   /**< Number of sectors to read */
);

hal_disk_result_t
hal_disk_write(uint8_t pdrv,        /**< Physical drive number */
               const uint8_t *buff, /**< Data to be written */
               uint32_t sector,     /**< Sector address (LBA) */
               uint32_t count       /**< Number of sectors to write */
);

/* Generic IOCTL commands */
#define HAL_DISK_IO_SYNC 0
#define HAL_DISK_IO_GET_SECTOR_COUNT 1
#define HAL_DISK_IO_GET_SECTOR_SIZE 2
#define HAL_DISK_IO_GET_BLOCK_SIZE 3

hal_disk_result_t hal_disk_ioctl(uint8_t pdrv, uint8_t cmd, void *buff);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // !HAL_DISKIO_H
