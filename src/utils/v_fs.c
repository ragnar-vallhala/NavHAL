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
 * @file v_fs.c
 * @brief Implementation of POSIX-like filesystem API using FatFS.
 */

#include "utils/v_fs.h"
#include "fatfs/ff.h"

#define MAX_OPEN_FILES 4
#define MAX_OPEN_DIRS 2

static FATFS fs_obj;
static FIL open_files[MAX_OPEN_FILES];
static uint8_t file_in_use[MAX_OPEN_FILES] = {0};
static DIR dir_objs[MAX_OPEN_DIRS];
static uint8_t dir_in_use[MAX_OPEN_DIRS] = {0};

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

  if (fd == -1) {
    return -1;
  }

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
  /* Free the slot UNCONDITIONALLY: f_close invalidates the FIL even on an error
   * (e.g. a sync write fault), so keeping file_in_use[fd]=1 would leak the slot
   * permanently and, after MAX_OPEN_FILES such errors, make every subsequent
   * open fail. Report the error, but never strand the descriptor. */
  file_in_use[fd] = 0;
  return (res == FR_OK) ? 0 : -(int)res;
}

int v_read(v_fd_t fd, void *buf, size_t count) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  unsigned int br;
  FRESULT res = f_read(&open_files[fd], buf, (unsigned int)count, &br);
  if (res == FR_OK)
    return (int)br;
  return -(int)res;
}

int v_write(v_fd_t fd, const void *buf, size_t count) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  unsigned int bw;
  FRESULT res = f_write(&open_files[fd], buf, (unsigned int)count, &bw);
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

int v_sync(v_fd_t fd) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !file_in_use[fd])
    return -1;

  FRESULT res = f_sync(&open_files[fd]);
  if (res == FR_OK)
    return 0;
  return -(int)res;
}

int v_preallocate(const char *path, uint32_t size) {
  /* Only create the file if it does not already exist.
   * FA_CREATE_NEW returns FR_EXIST when the file is present —
   * meaning we already pre-allocated it on a previous boot. */
  FIL fil;
  FRESULT res = f_open(&fil, path, FA_CREATE_NEW | FA_WRITE);
  if (res == FR_EXIST)
    return 0; /* Already pre-allocated — nothing to do */
  if (res != FR_OK)
    return -(int)res;

  /* Fill the file with zeros sector by sector.
   * This commits the full FAT cluster chain to the card NOW, at boot.
   * Subsequent writes seek in-place — no FAT modification needed. */
  static const uint8_t zero_buf[512] = {0};
  uint32_t remaining = size;
  while (remaining > 0) {
    UINT chunk = (remaining < sizeof(zero_buf)) ? remaining : sizeof(zero_buf);
    UINT bw = 0;
    res = f_write(&fil, zero_buf, chunk, &bw);
    if (res != FR_OK || bw != chunk) {
      f_close(&fil);
      return (res != FR_OK) ? -(int)res : -1;
    }
    remaining -= bw;
  }

  /* Flush FAT + directory entry before closing */
  f_sync(&fil);
  f_close(&fil);
  return 0;
}

int v_stat(const char *path, v_stat_t *st) {
  if (st == NULL)
    return -1;
  FILINFO fno;
  FRESULT res = f_stat(path, &fno);
  if (res != FR_OK) {
    st->exists = 0;
    return -(int)res;
  }
  st->exists = 1;
  st->size = (uint32_t)fno.fsize;
  st->is_dir = (fno.fattrib & AM_DIR) ? 1 : 0;
  st->mtime = ((uint32_t)fno.fdate << 16) | (uint32_t)fno.ftime;
  return 0;
}

v_dir_t v_opendir(const char *path) {
  int d = -1;
  for (int i = 0; i < MAX_OPEN_DIRS; i++) {
    if (!dir_in_use[i]) {
      d = i;
      break;
    }
  }
  if (d == -1)
    return -1;

  FRESULT res = f_opendir(&dir_objs[d], path);
  if (res != FR_OK)
    return -(int)res;

  dir_in_use[d] = 1;
  return d;
}

int v_readdir(v_dir_t d, v_dirent_t *ent) {
  if (d < 0 || d >= MAX_OPEN_DIRS || !dir_in_use[d] || ent == NULL)
    return -1;

  FILINFO fno;
  FRESULT res = f_readdir(&dir_objs[d], &fno);
  if (res != FR_OK)
    return -(int)res;
  if (fno.fname[0] == 0)
    return 0; /* end of directory */

  int i = 0;
  for (; i < V_NAME_MAX - 1 && fno.fname[i] != 0; i++)
    ent->name[i] = (char)fno.fname[i];
  ent->name[i] = '\0';
  ent->size = (uint32_t)fno.fsize;
  ent->is_dir = (fno.fattrib & AM_DIR) ? 1 : 0;
  return 1;
}

int v_closedir(v_dir_t d) {
  if (d < 0 || d >= MAX_OPEN_DIRS || !dir_in_use[d])
    return -1;
  FRESULT res = f_closedir(&dir_objs[d]);
  dir_in_use[d] = 0;
  return (res == FR_OK) ? 0 : -(int)res;
}
