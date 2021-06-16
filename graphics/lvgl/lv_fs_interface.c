/****************************************************************************
 * apps/graphics/lvgl/lv_fs_interface.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/statfs.h>
#include "lv_fs_interface.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LV_FS_LETTER '/'

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/* Create a type to store the required data about your file. */

typedef int file_t;

/* Similarly to `file_t` create a type for directory reading too */

typedef DIR *dir_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static lv_fs_res_t fs_open(lv_fs_drv_t *drv, void *file_p,
                           const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p,
                           void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p,
                            const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p,
                           uint32_t pos);
static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p,
                           uint32_t *size_p);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p,
                           uint32_t *pos_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_trunc(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname,
                             const char *newname);
static lv_fs_res_t fs_free(lv_fs_drv_t *drv, uint32_t *total_p,
                           uint32_t *free_p);
static lv_fs_res_t fs_dir_open(lv_fs_drv_t *drv, void *dir_p,
                               const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fs_open
 *
 * Description:
 *   Open a file.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   path   - path to the file beginning with the driver letter.
 *            (e.g. /folder/file.txt)
 *   mode   - read: FS_MODE_RD, write: FS_MODE_WR,
 *            both: FS_MODE_RD | FS_MODE_WR
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_open(lv_fs_drv_t *drv, void *file_p,
                           const char *path, lv_fs_mode_t mode)
{
  uint32_t flags = 0;
  if (mode == LV_FS_MODE_WR)
    {
      flags = O_WRONLY | O_CREAT;
    }
  else if (mode == LV_FS_MODE_RD)
    {
      flags = O_RDONLY;
    }
  else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    {
      flags = O_RDWR | O_CREAT;
    }
  else
    {
      return LV_FS_RES_UNKNOWN;
    }

  file_t f = open(--path, flags);
  if (f < 0)
    {
      return LV_FS_RES_FS_ERR;
    }

  /* 'file_p' is pointer to a file descriptor and
   * we need to store our file descriptor here
   */

  file_t *fp = file_p;        /* Just avoid the confusing casings */
  *fp = f;

  return LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_close
 *
 * Description:
 *   Close an opened file.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *
 * Returned Value:
 *   LV_FS_RES_OK: no error, the file is read
 *   any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  return close(*fp) < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_read
 *
 * Description:
 *   Read data from an opened file.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   buf    - pointer to a memory block where to store the read data.
 *   btr    - number of Bytes To Read.
 *   br     - the real number of read bytes (Byte Read).
 *
 * Returned Value:
 *   LV_FS_RES_OK: no error, the file is read
 *   any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p,
                           void *buf, uint32_t btr, uint32_t *br)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  *br = read(*fp, buf, btr);

  return (int32_t)*br < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_write
 *
 * Description:
 *   Write into a file.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   buf    - pointer to a buffer with the bytes to write.
 *   btw    - Bytes To Write.
 *   bw     - the number of real written bytes (Bytes Written).
 *            NULL if unused.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p,
                            const void *buf, uint32_t btw, uint32_t *bw)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  *bw = write(*fp, buf, btw);

  return (int32_t)*bw < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_seek
 *
 * Description:
 *   Set the read write pointer. Also expand the file size if necessary.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   pos    - the new position of read write pointer.
 *
 * Returned Value:
 *   LV_FS_RES_OK: no error, the file is read
 *   any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  off_t offset = lseek(*fp, pos, SEEK_SET);

  return offset < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_size
 *
 * Description:
 *   Give the size of a file bytes.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   size   - pointer to a variable to store the size.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p,
                           uint32_t *size_p)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  off_t cur = lseek(*fp, 0, SEEK_CUR);

  *size_p = lseek(*fp, 0L, SEEK_END);

  /* Restore file pointer */

  lseek(*fp, cur, SEEK_SET);

  return (int32_t)*size_p < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_tell
 *
 * Description:
 *   Give the position of the read write pointer.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *   pos_p  - pointer to to store the result.
 *
 * Returned Value:
 *   LV_FS_RES_OK: no error, the file is read
 *   any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p,
                           uint32_t *pos_p)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  *pos_p = lseek(*fp, 0, SEEK_CUR);

  return (int32_t)*pos_p < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_remove
 *
 * Description:
 *   Delete a file.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   path   - path of the file to delete.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path)
{
  return remove(--path) < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_trunc
 *
 * Description:
 *   Truncate the file size to the current position of
 *   the read write pointer.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   file_p - pointer to a file_t variable.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_trunc(lv_fs_drv_t *drv, void *file_p)
{
  /* Just avoid the confusing casings */

  file_t *fp = file_p;

  off_t p = lseek(*fp, 0, SEEK_CUR);

  return ftruncate(*fp, p) < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_rename
 *
 * Description:
 *   Rename a file.
 *
 * Input Parameters:
 *   drv     - pointer to a driver where this function belongs.
 *   oldname - path to the file.
 *   newname - path with the new name
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname,
                             const char *newname)
{
  return rename(--oldname, --newname) < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_free
 *
 * Description:
 *   Get the free and total size of a driver in kB.
 *
 * Input Parameters:
 *   drv     - pointer to a driver where this function belongs.
 *   total_p - pointer to store the total size [kB].
 *   free_p  - pointer to store the free size [kB]
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_free(lv_fs_drv_t *drv, uint32_t *total_p,
                           uint32_t *free_p)
{
  struct statfs sfs;

  if (statfs(CONFIG_LV_FILESYSTEM_MOUNTPOINT, &sfs) < 0)
    {
      return LV_FS_RES_FS_ERR;
    }
  else
    {
      *total_p = sfs.f_blocks * sfs.f_bsize / 1024;
      *free_p = sfs.f_bfree * sfs.f_bsize / 1024;
      return LV_FS_RES_OK;
    }
}

/****************************************************************************
 * Name: fs_dir_open
 *
 * Description:
 *   Initialize a 'fs_read_dir_t' variable for directory reading.
 *
 * Input Parameters:
 *   drv     - pointer to a driver where this function belongs.
 *   dir_p   - pointer to a 'fs_read_dir_t' variable.
 *   path    - path to a directory.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_dir_open(lv_fs_drv_t *drv, void *dir_p,
                               const char *path)
{
  dir_t d;

  /* Make the path relative to the current directory
   * (the projects root folder)
   */

  if ((d = opendir(--path)) == NULL)
    {
      return LV_FS_RES_FS_ERR;
    }
  else
    {
      /* 'dir_p' is pointer to a file descriptor and
       * we need to store our file descriptor here
       */

      /* Just avoid the confusing casings */

      dir_t *dp = dir_p;
      *dp = d;
    }

  return LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_dir_read
 *
 * Description:
 *   Read the next filename form a directory.
 *   The name of the directories will begin with '/'.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   dir_p  - pointer to an initialized 'fs_read_dir_t' variable.
 *   fn     - pointer to a buffer to store the filename.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn)
{
  /* Just avoid the confusing casings */

  dir_t *dp = dir_p;

  do
    {
      struct dirent *entry = readdir(*dp);

      if (entry)
        {
          if (entry->d_type == DT_DIR)
            {
              sprintf(fn, "/%s", entry->d_name);
            }
          else
            {
              strcpy(fn, entry->d_name);
            }
        }
      else
        {
          strcpy(fn, "");
        }
    }
  while (strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);

  return LV_FS_RES_OK;
}

/****************************************************************************
 * Name: fs_dir_read
 *
 * Description:
 *   Close the directory reading.
 *
 * Input Parameters:
 *   drv    - pointer to a driver where this function belongs.
 *   dir_p  - pointer to an initialized 'fs_read_dir_t' variable.
 *
 * Returned Value:
 *   LV_FS_RES_OK or any error from lv_fs_res_t enum.
 *
 ****************************************************************************/

static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p)
{
  dir_t *dp = dir_p;

  return closedir(*dp) < 0 ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_fs_interface_init
 *
 * Description:
 *   Register a driver for the File system interface.
 *
 ****************************************************************************/

void lv_fs_interface_init(void)
{
  /* Add a simple drive to open images */

  lv_fs_drv_t fs_drv; /* A driver descriptor */

  lv_fs_drv_init(&fs_drv);

  /* Set up fields... */

  fs_drv.file_size = sizeof(file_t);
  fs_drv.letter = LV_FS_LETTER;
  fs_drv.open_cb = fs_open;
  fs_drv.close_cb = fs_close;
  fs_drv.read_cb = fs_read;
  fs_drv.write_cb = fs_write;
  fs_drv.seek_cb = fs_seek;
  fs_drv.tell_cb = fs_tell;
  fs_drv.free_space_cb = fs_free;
  fs_drv.size_cb = fs_size;
  fs_drv.remove_cb = fs_remove;
  fs_drv.rename_cb = fs_rename;
  fs_drv.trunc_cb = fs_trunc;

  fs_drv.rddir_size = sizeof(dir_t);
  fs_drv.dir_close_cb = fs_dir_close;
  fs_drv.dir_open_cb = fs_dir_open;
  fs_drv.dir_read_cb = fs_dir_read;

  lv_fs_drv_register(&fs_drv);
}
