/**
 * @file v_fs.c
 * @brief Implementation of POSIX-like filesystem API using FatFS.
 */

#include "utils/v_fs.h"
#include "fatfs/ff.h"

#define MAX_OPEN_FILES 4

static FATFS fs_obj;
static FIL open_files[MAX_OPEN_FILES];
static uint8_t file_in_use[MAX_OPEN_FILES] = {0};

int v_fs_init(void) {
  FRESULT res = f_mount(&fs_obj, "0:", 1);
  if (res != FR_OK) {
    return -(int)res;
  }
  return 0;
}

v_fd_t v_open(const char *path, int flags) {
  int fd = -1;
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (!file_in_use[i]) {
      fd = i;
      break;
    }
  }

  if (fd == -1)
    return -1;

  uint8_t mode = 0;
  if (flags & V_O_RDONLY)
    mode |= FA_READ;
  if (flags & V_O_WRONLY)
    mode |= FA_WRITE;
  if (flags & V_O_RDWR)
    mode |= (FA_READ | FA_WRITE);
  if (flags & V_O_CREAT)
    mode |= FA_OPEN_ALWAYS;
  if (flags & V_O_TRUNC)
    mode |= FA_CREATE_ALWAYS;
  if (flags & V_O_APPEND)
    mode |= FA_OPEN_APPEND;

  FRESULT res = f_open(&open_files[fd], path, mode);
  if (res != FR_OK) {
    return -(int)res;
  }

  file_in_use[fd] = 1;
  return fd;
}

int v_close(v_fd_t fd) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  FRESULT res = f_close(&open_files[fd]);
  if (res == FR_OK) {
    file_in_use[fd] = 0;
    return 0;
  }
  return -(int)res;
}

int v_read(v_fd_t fd, void *buf, size_t count) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  uint32_t br;
  FRESULT res = f_read(&open_files[fd], buf, (uint32_t)count, &br);
  if (res == FR_OK)
    return (int)br;
  return -(int)res;
}

int v_write(v_fd_t fd, const void *buf, size_t count) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  uint32_t bw;
  FRESULT res = f_write(&open_files[fd], buf, (uint32_t)count, &bw);
  if (res == FR_OK)
    return (int)bw;
  return -(int)res;
}

long v_lseek(v_fd_t fd, long offset, int whence) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  uint32_t target_pos = 0;
  switch (whence) {
  case V_SEEK_SET:
    target_pos = (uint32_t)offset;
    break;
  case V_SEEK_CUR:
    target_pos = f_tell(&open_files[fd]) + (uint32_t)offset;
    break;
  case V_SEEK_END:
    target_pos = f_size(&open_files[fd]) + (uint32_t)offset;
    break;
  default:
    return -1;
  }

  FRESULT res = f_lseek(&open_files[fd], target_pos);
  if (res == FR_OK)
    return (long)f_tell(&open_files[fd]);
  return -(int)res;
}

int v_mkdir(const char *path) {
  FRESULT res = f_mkdir(path);
  if (res == FR_OK)
    return 0;
  return -(int)res;
}

int v_unlink(const char *path) {
  FRESULT res = f_unlink(path);
  if (res == FR_OK)
    return 0;
  return -(int)res;
}
