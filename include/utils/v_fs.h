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
 * @file v_fs.h
 * @brief POSIX-like filesystem API for NavHAL.
 */

#ifndef V_FS_H
#define V_FS_H

/**
 * @defgroup HAL_UTIL_VFS Vfs
 * @ingroup HAL_UTILS
 * @brief Lightweight virtual filesystem glue.
 * @{
 */

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/* File access modes */
#define V_O_RDONLY 0x01
#define V_O_WRONLY 0x02
#define V_O_RDWR 0x03
#define V_O_CREAT 0x04
#define V_O_TRUNC 0x08
#define V_O_APPEND 0x10

/* Seek origins */
#define V_SEEK_SET 0
#define V_SEEK_CUR 1
#define V_SEEK_END 2

typedef int v_fd_t;
typedef int v_dir_t;

/* FatFS short (8.3) names: 12 chars + NUL (FF_USE_LFN = 0). */
#define V_NAME_MAX 13

/** @brief One directory entry returned by v_readdir(). */
typedef struct {
  char name[V_NAME_MAX]; /**< file/dir name (8.3) */
  uint32_t size;         /**< file size in bytes (0 for directories) */
  uint8_t is_dir;        /**< 1 if this entry is a directory */
} v_dirent_t;

/** @brief File/directory status returned by v_stat(). */
typedef struct {
  uint32_t size;  /**< file size in bytes (0 for directories) */
  uint32_t mtime; /**< FatFS packed modified time: (fdate << 16) | ftime */
  uint8_t is_dir; /**< 1 if the path is a directory */
  uint8_t exists; /**< 1 if the path exists (else the rest is unset) */
} v_stat_t;

/**
 * @brief Initialize the filesystem and mount the default drive.
 * @return 0 on success, negative error code otherwise.
 */
int v_fs_init(void);

/**
 * @brief Open a file.
 * @param path Path to the file.
 * @param flags Access flags (V_O_...).
 * @return File descriptor on success, negative error code otherwise.
 */
v_fd_t v_open(const char *path, int flags);

/**
 * @brief Close an open file.
 * @param fd File descriptor.
 * @return 0 on success, negative error code otherwise.
 */
int v_close(v_fd_t fd);

/**
 * @brief Read data from a file.
 * @param fd File descriptor.
 * @param buf Buffer to store data.
 * @param count Number of bytes to read.
 * @return Number of bytes read on success, negative error code otherwise.
 */
int v_read(v_fd_t fd, void *buf, size_t count);

/**
 * @brief Write data to a file.
 * @param fd File descriptor.
 * @param buf Data to write.
 * @param count Number of bytes to write.
 * @return Number of bytes written on success, negative error code otherwise.
 */
int v_write(v_fd_t fd, const void *buf, size_t count);

/**
 * @brief Set file position.
 * @param fd File descriptor.
 * @param offset Offset from origin.
 * @param whence Origin (V_SEEK_...).
 * @return New position on success, negative error code otherwise.
 */
long v_lseek(v_fd_t fd, long offset, int whence);

/**
 * @brief Create a directory.
 */
int v_mkdir(const char *path);

/**
 * @brief Delete a file or directory.
 */
int v_unlink(const char *path);

/**
 * @brief Flush cached data to a file.
 * @param fd File descriptor.
 * @return 0 on success, negative error code otherwise.
 */
int v_sync(v_fd_t fd);

/**
 * @brief Pre-allocate a file with contiguous sectors on first boot.
 *
 * If the file already exists this is a no-op. Otherwise the file is
 * created and f_expand() is used to reserve `size` bytes of contiguous
 * space so that subsequent in-place writes never modify the FAT chain,
 * giving near-crash-safe behaviour for sequential logs.
 *
 * @param path Path to the file (with drive prefix, e.g. "0:log.dat").
 * @param size Number of bytes to pre-allocate.
 * @return 0 on success, negative FatFS error code otherwise.
 */
int v_preallocate(const char *path, uint32_t size);

/**
 * @brief Get the status of a file or directory.
 * @param path Path (with drive prefix, e.g. "0:log.dat").
 * @param st   Filled on return; st->exists distinguishes "absent" from "error".
 * @return 0 on success (path exists), negative FatFS error code otherwise
 *         (st->exists set to 0).
 */
int v_stat(const char *path, v_stat_t *st);

/**
 * @brief Open a directory for iteration.
 * @param path Directory path (e.g. "0:" or "0:logs").
 * @return A directory handle (>= 0) on success, negative error code otherwise.
 */
v_dir_t v_opendir(const char *path);

/**
 * @brief Read the next entry from an open directory.
 * @param d   Directory handle from v_opendir().
 * @param ent Filled with the next entry on return.
 * @return 1 if an entry was read, 0 at end of directory, negative on error.
 */
int v_readdir(v_dir_t d, v_dirent_t *ent);

/**
 * @brief Close a directory handle.
 * @return 0 on success, negative error code otherwise.
 */
int v_closedir(v_dir_t d);


#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_UTIL_VFS */
#endif // V_FS_H
