/**
 * @file v_fs.h
 * @brief POSIX-like filesystem API for NavHAL.
 */

#ifndef V_FS_H
#define V_FS_H

#include <stddef.h>
#include <stdint.h>

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

#endif // V_FS_H
