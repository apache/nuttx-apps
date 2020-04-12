/****************************************************************************
 * examples/userfs/userfs_main.c
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

#include <nuttx/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <nuttx/fs/userfs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define UFSTEST_NFILES       3
#define UFSTEST_FS_BLOCKSIZE 32

#define UFSTEST_FILE1_BLOCKS (2)
#define UFSTEST_FILE1_MXSIZE (UFSTEST_FILE1_BLOCKS * UFSTEST_FS_BLOCKSIZE)
#define UFSTEST_FILE2_BLOCKS (3)
#define UFSTEST_FILE2_MXSIZE (UFSTEST_FILE2_BLOCKS * UFSTEST_FS_BLOCKSIZE)
#define UFSTEST_FILE3_BLOCKS (4)
#define UFSTEST_FILE3_MXSIZE (UFSTEST_FILE3_BLOCKS * UFSTEST_FS_BLOCKSIZE)

#define UFSTEST_FS_NBLOCKS   (UFSTEST_FILE1_BLOCKS + UFSTEST_FILE2_BLOCKS + UFSTEST_FILE3_BLOCKS)
#define UFSTEST_FS_NBYTES    (UFSTEST_FILE1_MXSIZE + UFSTEST_FILE2_MXSIZE + UFSTEST_FILE3_MXSIZE)
#define UFSTEST_MXWRITE      UFSTEST_FILE3_MXSIZE

#define UFSTEST_MOUNTPOUNT   "/mnt/ufstest"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ufstest_file_s
{
  struct dirent entry;
  uint16_t inuse;
  uint16_t mxsize;
  FAR char *data;
};

struct ufstest_openfile_s
{
  int16_t pos;
  int16_t crefs;
  FAR struct ufstest_file_s *file;
};

struct ufstest_opendir_s
{
  int16_t index;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* UserFS methods */

static int     ufstest_open(FAR void *volinfo, FAR const char *relpath,
                 int oflags, mode_t mode, FAR void **openinfo);
static int     ufstest_close(FAR void *volinfo, FAR void *openinfo);
static ssize_t ufstest_read(FAR void *volinfo, FAR void *openinfo,
                 FAR char *buffer, size_t buflen);
static ssize_t ufstest_write(FAR void *volinfo, FAR void *openinfo,
                 FAR const char *buffer, size_t buflen);
static off_t   ufstest_seek(FAR void *volinfo, FAR void *openinfo,
                 off_t offset, int whence);
static int     ufstest_ioctl(FAR void *volinfo, FAR void *openinfo, int cmd,
                 unsigned long arg);
static int     ufstest_sync(FAR void *volinfo, FAR void *openinfo);
static int     ufstest_dup(FAR void *volinfo, FAR void *oldinfo,
                 FAR void **newinfo);
static int     ufstest_fstat(FAR void *volinfo, FAR void *openinfo,
                 FAR struct stat *buf);
static int     ufstest_truncate(FAR void *volinfo, FAR void *openinfo,
                 off_t length);
static int     ufstest_opendir(FAR void *volinfo, FAR const char *relpath,
                 FAR void **dir);
static int     ufstest_closedir(FAR void *volinfo, FAR void *dir);
static int     ufstest_readdir(FAR void *volinfo, FAR void *dir,
                 FAR struct dirent *entry);
static int     ufstest_rewinddir(FAR void *volinfo, FAR void *dir);
static int     ufstest_statfs(FAR void *volinfo, FAR struct statfs *buf);
static int     ufstest_unlink(FAR void *volinfo, FAR const char *relpath);
static int     ufstest_mkdir(FAR void *volinfo, FAR const char *relpath,
                 mode_t mode);
static int     ufstest_rmdir(FAR void *volinfo, FAR const char *relpath);
static int     ufstest_rename(FAR void *volinfo, FAR const char *oldrelpath,
                 FAR const char *newrelpath);
static int     ufstest_stat(FAR void *volinfo, FAR const char *relpath,
                 FAR struct stat *buf);
static int     ufstest_destroy(FAR void *volinfo);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_file1_data[UFSTEST_FILE1_MXSIZE] = "This is file 1";
#define UFSTEST_INIT_FILE1_SIZE 14

static char g_file2_data[UFSTEST_FILE2_MXSIZE] = "This is file 2";
#define UFSTEST_INIT_FILE2_SIZE 14

static char g_file3_data[UFSTEST_FILE3_MXSIZE] = "This is file 3";
#define UFSTEST_INIT_FILE3_SIZE 14

static struct ufstest_file_s g_rootdir[UFSTEST_NFILES] =
{
    {
      { DTYPE_FILE, "File1" },
      UFSTEST_INIT_FILE1_SIZE,
      UFSTEST_FILE1_MXSIZE,
      g_file1_data
    },
    {
      { DTYPE_FILE, "File2" },
      UFSTEST_INIT_FILE2_SIZE,
      UFSTEST_FILE2_MXSIZE,
      g_file2_data
    },
    {
      { DTYPE_FILE, "File3" },
      UFSTEST_INIT_FILE3_SIZE,
      UFSTEST_FILE3_MXSIZE,
      g_file3_data
    }
};

static const struct userfs_operations_s g_ufstest_ops =
{
  ufstest_open,
  ufstest_close,
  ufstest_read,
  ufstest_write,
  ufstest_seek,
  ufstest_ioctl,
  ufstest_sync,
  ufstest_dup,
  ufstest_fstat,
  ufstest_truncate,
  ufstest_opendir,
  ufstest_closedir,
  ufstest_readdir,
  ufstest_rewinddir,
  ufstest_statfs,
  ufstest_unlink,
  ufstest_mkdir,
  ufstest_rmdir,
  ufstest_rename,
  ufstest_stat,
  ufstest_destroy
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ufstest_findbyname
 ****************************************************************************/

static FAR struct ufstest_file_s *ufstest_findbyname(FAR const char *relpath)
{
  int i;

  for (i = 0; i < UFSTEST_NFILES; i++)
    {
      if (strcmp(relpath, g_rootdir[i].entry.d_name) == 0)
        {
          return &g_rootdir[i];
        }
    }

  return NULL;
}

/****************************************************************************
 * UserFS methods
 ****************************************************************************/

static int ufstest_open(FAR void *volinfo, FAR const char *relpath,
                        int oflags, mode_t mode, FAR void **openinfo)
{
  FAR struct ufstest_openfile_s *opriv;
  FAR struct ufstest_file_s *file;

  file = ufstest_findbyname(relpath);
  if (file != NULL)
    {
      opriv = (FAR struct ufstest_openfile_s *)
         malloc(sizeof(struct ufstest_openfile_s));
      if (opriv == NULL)
        {
          return -ENOMEM;
        }

      if ((oflags & O_TRUNC) != 0)
        {
          file->inuse = 0;
        }

      if ((oflags & (O_WROK | O_APPEND)) == (O_WROK | O_APPEND))
        {
          opriv->pos = file->inuse;
        }
      else
        {
          opriv->pos = 0;
        }

      opriv->file = file;

      /* Initiallly, there is one reference count on the open data.  This may
       * be incremented in the event that the file is dup'ed.
       */

      opriv->crefs = 1;

      /* Return the opaque reference to the open data */

      *openinfo = opriv;
      return OK;
    }

  return -ENOENT;
}

static int ufstest_close(FAR void *volinfo, FAR void *openinfo)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)openinfo;

  if (opriv != NULL)
    {
      if (opriv->crefs <= 1)
        {
          free(opriv);
        }
      else
        {
          opriv->crefs--;
        }
    }

  return OK;
}

static ssize_t ufstest_read(FAR void *volinfo, FAR void *openinfo,
                            FAR char *buffer, size_t buflen)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)openinfo;
  ssize_t readsize;

  readsize = opriv->file->inuse - opriv->pos;
  if (readsize > buflen)
    {
      readsize = buflen;
    }

  if (readsize > 0)
    {
      memcpy(buffer, &opriv->file->data[opriv->pos], readsize);
      opriv->pos += readsize;
    }

  return readsize <= 0 ? 0 : readsize;
}

static ssize_t ufstest_write(FAR void *volinfo, FAR void *openinfo,
                             FAR const char *buffer, size_t buflen)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)openinfo;
  ssize_t writesize;

  writesize = opriv->file->mxsize - opriv->pos;
  if (writesize > buflen)
    {
      writesize = buflen;
    }

  memcpy(&opriv->file->data[opriv->pos], buffer, writesize);
  opriv->pos += writesize;
  if (opriv->pos > opriv->file->inuse)
    {
      opriv->file->inuse = opriv->pos;
    }

  return writesize;
}

static off_t ufstest_seek(FAR void *volinfo, FAR void *openinfo,
                          off_t offset, int whence)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)openinfo;
  off_t newpos;

  switch (whence)
    {
    case SEEK_SET: /* The offset is set to offset bytes. */
        newpos = offset;
        break;

    case SEEK_CUR: /* The offset is set to its current location plus
                    * offset bytes. */

        newpos = offset + opriv->pos;
        break;

    case SEEK_END: /* The offset is set to the size of the file plus
                    * offset bytes. */

        newpos = offset + opriv->file->inuse;
        break;

    default:
        fprintf(stderr, "ERROR: Whence is invalid: %d\n", whence);
        return -EINVAL;
    }

  if (newpos < 0)
    {
      newpos = 0;
    }
  else if (newpos > opriv->file->mxsize)
    {
      newpos = opriv->file->mxsize;
    }

  opriv->pos = newpos;
  return newpos;
}

static int ufstest_ioctl(FAR void *volinfo, FAR void *openinfo, int cmd,
                         unsigned long arg)
{
  return -ENOTTY;
}

static int ufstest_sync(FAR void *volinfo, FAR void *openinfo)
{
  return OK;
}

static int ufstest_dup(FAR void *volinfo, FAR void *oldinfo,
                       FAR void **newinfo)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)oldinfo;

  /* Increment the reference count on the open info */

  opriv->crefs++;

  /* And just copy the openinfo reference */

  *newinfo = oldinfo;
  return OK;
}

static int ufstest_fstat(FAR void *volinfo, FAR void *openinfo,
                         FAR struct stat *buf)
{
  FAR struct ufstest_openfile_s *opriv =
    (FAR struct ufstest_openfile_s *)openinfo;

  buf->st_mode    = (S_IFREG | S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  buf->st_size    = opriv->file->inuse;
  buf->st_blksize = UFSTEST_FS_BLOCKSIZE;
  buf->st_blocks  = (opriv->file->inuse + UFSTEST_FS_BLOCKSIZE - 1) /
                    UFSTEST_FS_BLOCKSIZE;
  buf->st_atime   = 0;
  buf->st_mtime   = 0;
  buf->st_ctime   = 0;
  return OK;
}

static int ufstest_truncate(FAR void *volinfo, FAR void *openinfo,
                            off_t length)
{
  return -ENOSYS;
}

static int ufstest_opendir(FAR void *volinfo, FAR const char *relpath,
                           FAR void **dir)
{
  FAR struct ufstest_opendir_s *odir;

  if (!relpath || relpath[0] == '\0')
    {
      /* The path refers to the top level directory. */

      odir = (FAR struct ufstest_opendir_s *)
         malloc(sizeof(struct ufstest_opendir_s));
      if (odir == NULL)
        {
          return -ENOMEM;
        }

      odir->index = 0;
      *dir = odir;
      return OK;
    }

  return -ENOENT;
}

static int ufstest_closedir(FAR void *volinfo, FAR void *dir)
{
  if (dir != NULL)
    {
      free(dir);
    }

  return OK;
}

static int ufstest_readdir(FAR void *volinfo, FAR void *dir,
                           FAR struct dirent *entry)
{
  FAR struct ufstest_file_s *priv = (FAR struct ufstest_file_s *)volinfo;
  FAR struct ufstest_opendir_s *odir = (FAR struct ufstest_opendir_s *)dir;

  if (odir->index < UFSTEST_NFILES)
    {
      memcpy(entry, &priv[odir->index].entry, sizeof(struct dirent));
      odir->index++;
      return OK;
    }

  return -ENOENT;
}

static int ufstest_rewinddir(FAR void *volinfo, FAR void *dir)
{
  FAR struct ufstest_opendir_s *odir = (FAR struct ufstest_opendir_s *)dir;

  odir->index = 0;
  return OK;
}

static int ufstest_statfs(FAR void *volinfo, FAR struct statfs *buf)
{
  int inuse = 0;
  int i;

  for (i = 0; i < UFSTEST_NFILES; i++)
    {
      inuse += (g_rootdir[i].inuse + UFSTEST_FS_BLOCKSIZE - 1) /
               UFSTEST_FS_BLOCKSIZE;
    }

  buf->f_type    = USERFS_MAGIC;
  buf->f_namelen = NAME_MAX;
  buf->f_bsize   = UFSTEST_FS_BLOCKSIZE;
  buf->f_blocks  = UFSTEST_FS_NBLOCKS;
  buf->f_bfree   = UFSTEST_FS_NBLOCKS - inuse;
  buf->f_bavail  = UFSTEST_FS_NBLOCKS - inuse;
  buf->f_files   = UFSTEST_NFILES;
  buf->f_ffree   = 0;
  return OK;
}

static int ufstest_unlink(FAR void *volinfo, FAR const char *relpath)
{
  return -ENOSYS;
}

static int ufstest_mkdir(FAR void *volinfo, FAR const char *relpath,
                         mode_t mode)
{
  return -ENOSYS;
}

static int ufstest_rmdir(FAR void *volinfo, FAR const char *relpath)
{
  return -ENOSYS;
}

static int ufstest_rename(FAR void *volinfo, FAR const char *oldrelpath,
                          FAR const char *newrelpath)
{
  FAR struct ufstest_file_s *file;

  file = ufstest_findbyname(oldrelpath);
  if (file != NULL)
    {
      strncpy(file->entry.d_name, newrelpath, NAME_MAX + 1);
      return OK;
    }

  return -ENOENT;
}

static int ufstest_stat(FAR void *volinfo, FAR const char *relpath,
                        FAR struct stat *buf)
{
  FAR void *openinfo;
  int ret;

  /* Are we stat'ing the directory?  Of one of the files in the directory? */

  if (*relpath == '\0')
    {
      memset(buf, 0, sizeof(struct stat));
      buf->st_mode    = (S_IFDIR | S_IRWXU | S_IRUSR | S_IRGRP | S_IRWXG |
                         S_IROTH | S_IRWXO);
      buf->st_blksize = UFSTEST_FS_BLOCKSIZE;

      ret = OK;
    }
  else
    {
      ret = ufstest_open(volinfo, relpath, O_RDWR, 0644, &openinfo);
      if (ret >= 0)
        {
          ret = ufstest_fstat(volinfo, openinfo, buf);
          ufstest_close(volinfo, openinfo);
        }
    }

  return ret;
}

static int ufstest_destroy(FAR void *volinfo)
{
  return OK;
}

/****************************************************************************
 * ufstest_daemon
 ****************************************************************************/

int ufstest_daemon(int argc, char *argv[])
{
  int ret;

  ret = userfs_run(UFSTEST_MOUNTPOUNT, &g_ufstest_ops, g_rootdir,
                   UFSTEST_MXWRITE);

  fprintf(stderr, "ERROR: userfs_run() returned: %d\n", ret);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * userfs_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *nshargv[1];
  int pid;

  /* Spawn the UserFS test daemon */

  nshargv[0] = NULL;
  pid = task_create("UserFS", CONFIG_EXAMPLES_USERFS_PRIORITY,
                    CONFIG_EXAMPLES_USERFS_STACKSIZE, ufstest_daemon,
                    (FAR char * const *)nshargv);
  if (pid < 0)
    {
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
