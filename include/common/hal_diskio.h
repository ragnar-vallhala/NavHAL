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

#endif // !HAL_DISKIO_H
