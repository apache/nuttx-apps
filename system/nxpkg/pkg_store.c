/****************************************************************************
 * apps/system/nxpkg/pkg_store.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pkg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PKG_LOCK_OWNER "owner"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkg_lock_read_owner(FAR const char *path, FAR pid_t *owner)
{
  char owner_path[PATH_MAX];
  char record[32];
  char extra;
  long parsed_owner;
  ssize_t nread;
  int fd;
  int ret;

  ret = snprintf(owner_path, sizeof(owner_path), "%s/%s", path,
                 PKG_LOCK_OWNER);
  if (ret < 0 || (size_t)ret >= sizeof(owner_path))
    {
      return ret < 0 ? ret : -ENAMETOOLONG;
    }

  fd = open(owner_path, O_RDONLY);
  if (fd < 0)
    {
      return -errno;
    }

  nread = read(fd, record, sizeof(record) - 1);
  if (nread < 0)
    {
      ret = -errno;
      close(fd);
      return ret;
    }

  close(fd);
  record[nread] = '\0';
  if (sscanf(record, "%ld%c", &parsed_owner, &extra) != 1 ||
      parsed_owner <= 0 ||
      (long)(pid_t)parsed_owner != parsed_owner)
    {
      return -EINVAL;
    }

  *owner = (pid_t)parsed_owner;
  return 0;
}

static int pkg_store_format(FAR char *buffer, size_t size,
                            FAR const char *fmt,
                            FAR const char *name,
                            FAR const char *version)
{
  int ret;

  ret = snprintf(buffer, size, fmt, name, version);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= size)
    {
      return -ENAMETOOLONG;
    }

  return 0;
}

static int pkg_store_mkdir(FAR const char *path)
{
  struct stat st;
  int ret;

  ret = stat(path, &st);
  if (ret == 0)
    {
      if (!S_ISDIR(st.st_mode))
        {
          return -ENOTDIR;
        }

      return 0;
    }

  if (errno != ENOENT)
    {
      return -errno;
    }

  ret = mkdir(path, 0755);
  if (ret < 0 && errno != EEXIST)
    {
      return -errno;
    }

  return 0;
}

static int pkg_store_mkdirs(FAR const char *path)
{
  char buffer[PATH_MAX];
  FAR char *cursor;
  int ret;

  ret = snprintf(buffer, sizeof(buffer), "%s", path);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= sizeof(buffer))
    {
      return -ENAMETOOLONG;
    }

  for (cursor = buffer + 1; *cursor != '\0'; cursor++)
    {
      if (*cursor != '/')
        {
          continue;
        }

      *cursor = '\0';
      ret = pkg_store_mkdir(buffer);
      *cursor = '/';
      if (ret < 0)
        {
          return ret;
        }
    }

  return pkg_store_mkdir(buffer);
}

static int pkg_store_write_all(int fd, FAR const char *buffer, size_t length)
{
  size_t offset = 0;

  while (offset < length)
    {
      ssize_t ret;

      ret = write(fd, buffer + offset, length - offset);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      if (ret == 0)
        {
          return -EIO;
        }

      offset += (size_t)ret;
    }

  return 0;
}

static FAR const char *pkg_store_basename(FAR const char *path)
{
  FAR const char *leaf;

  leaf = strrchr(path, '/');
  return leaf != NULL ? leaf + 1 : path;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int pkg_store_prepare_layout(void)
{
  int ret;

  ret = pkg_store_mkdirs(PKG_REPO_DIR);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_mkdirs(PKG_STORE_DIR);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_mkdirs(PKG_TMP_DIR);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_store_mkdirs(PKG_TMP_PKG_DIR);
}

int pkg_lock_create(FAR const char *path)
{
  char owner_path[PATH_MAX];
  char record[32];
  int fd;
  int ret;

  if (mkdir(path, 0755) < 0)
    {
      return -errno;
    }

  ret = snprintf(owner_path, sizeof(owner_path), "%s/%s", path,
                 PKG_LOCK_OWNER);
  if (ret < 0 || (size_t)ret >= sizeof(owner_path))
    {
      ret = ret < 0 ? ret : -ENAMETOOLONG;
      rmdir(path);
      return ret;
    }

  fd = open(owner_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd < 0)
    {
      ret = -errno;
      rmdir(path);
      return ret;
    }

  ret = snprintf(record, sizeof(record), "%ld", (long)getpid());
  if (ret < 0 || (size_t)ret >= sizeof(record))
    {
      ret = ret < 0 ? ret : -EOVERFLOW;
    }
  else
    {
      ret = pkg_store_write_all(fd, record, (size_t)ret);
    }

  if (ret == 0 && fsync(fd) < 0)
    {
      ret = -errno;
    }

  if (close(fd) < 0 && ret == 0)
    {
      ret = -errno;
    }

  if (ret < 0)
    {
      unlink(owner_path);
      rmdir(path);
    }

  return ret;
}

void pkg_lock_remove(FAR const char *path)
{
  char owner_path[PATH_MAX];
  int ret;

  ret = snprintf(owner_path, sizeof(owner_path), "%s/%s", path,
                 PKG_LOCK_OWNER);
  if (ret >= 0 && (size_t)ret < sizeof(owner_path))
    {
      unlink(owner_path);
    }

  rmdir(path);
}

void pkg_reclaim_stale_lock(FAR const char *path)
{
  pid_t owner;
  int ret;

  ret = pkg_lock_read_owner(path, &owner);
  if (ret == -ENOENT || ret == -EINVAL)
    {
      /* The creator may not have written the owner marker yet. */

      usleep(20 * 1000);
      ret = pkg_lock_read_owner(path, &owner);
      if (ret == -ENOENT || ret == -EINVAL)
        {
          pkg_lock_remove(path);
          return;
        }
    }

  if (ret < 0)
    {
      return;
    }

  if (owner == getpid())
    {
      pkg_lock_remove(path);
      return;
    }

  if (kill(owner, 0) == 0 || errno == EPERM)
    {
      return;
    }

  if (errno == ESRCH)
    {
      pkg_error("reclaiming lock from exited task %ld '%s'",
                (long)owner, path);
      pkg_lock_remove(path);
    }
}

int pkg_store_ensure_package_root(FAR const char *name)
{
  char path[PATH_MAX];
  int ret;

  ret = pkg_store_format_package_root(path, sizeof(path), name);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_store_mkdirs(path);
}

int pkg_store_ensure_version_dir(FAR const char *name,
                                 FAR const char *version)
{
  char path[PATH_MAX];
  int ret;

  ret = pkg_store_ensure_package_root(name);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_format_version_path(path, sizeof(path), name, version);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_store_mkdirs(path);
}

int pkg_store_format_index_path(FAR char *buffer, size_t size)
{
  return pkg_store_format(buffer, size, "%s", PKG_REPO_INDEX, "");
}

int pkg_store_format_repo_source_path(FAR char *buffer, size_t size)
{
  return pkg_store_format(buffer, size, "%s", PKG_REPO_SOURCE, "");
}

int pkg_store_format_installed_path(FAR char *buffer, size_t size)
{
  return pkg_store_format(buffer, size, "%s", PKG_REPO_INSTALLED, "");
}

int pkg_store_format_package_root(FAR char *buffer, size_t size,
                                  FAR const char *name)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s", name, "");
}

int pkg_store_format_version_path(FAR char *buffer, size_t size,
                                  FAR const char *name,
                                  FAR const char *version)
{
  return pkg_store_format(buffer, size,
                          PKG_STORE_DIR "/%s/%s",
                          name, version);
}

int pkg_store_format_current_path(FAR char *buffer, size_t size,
                                  FAR const char *name)
{
  return pkg_store_format(buffer, size,
                          PKG_STORE_DIR "/%s/current",
                          name, "");
}

int pkg_store_format_previous_path(FAR char *buffer, size_t size,
                                   FAR const char *name)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/previous", name,
                          "");
}

int pkg_store_format_txn_path(FAR char *buffer, size_t size,
                              FAR const char *name)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/txn.tx", name,
                          "");
}

int pkg_store_format_lock_path(FAR char *buffer, size_t size,
                               FAR const char *name)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/lock.lk", name,
                          "");
}

int pkg_store_format_download_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version)
{
  int ret;

  UNUSED(name);
  UNUSED(version);

  /* Use the PID for a unique FAT 8.3-compatible staging name. */

  ret = snprintf(buffer, size, PKG_TMP_PKG_DIR "/dl%d.pkg", (int)getpid());
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= size)
    {
      return -ENAMETOOLONG;
    }

  return 0;
}

int pkg_store_format_payload_path(FAR char *buffer, size_t size,
                                  FAR const char *name,
                                  FAR const char *version,
                                  FAR const char *artifact)
{
  FAR const char *leaf;
  int ret;

  leaf = pkg_store_basename(artifact);
  ret = snprintf(buffer, size, PKG_STORE_DIR "/%s/%s/%s",
                 name, version, leaf);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= size)
    {
      return -ENAMETOOLONG;
    }

  return 0;
}

int pkg_store_format_manifest_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/%s/manifest.jsn",
                          name, version);
}

int pkg_store_read_text(FAR const char *path, FAR char **buffer)
{
  FAR char *data;
  struct stat st;
  size_t length;
  size_t nread;
  size_t total;
  int fd;

  if (buffer == NULL)
    {
      return -EINVAL;
    }

  *buffer = NULL;

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return errno == ENOENT ? -ENOENT : -errno;
    }

  if (fstat(fd, &st) < 0)
    {
      close(fd);
      return -errno;
    }

  if (!S_ISREG(st.st_mode))
    {
      close(fd);
      return -EINVAL;
    }

  /* Validate the size before allocating length plus its terminator. */

  if (st.st_size < 0 || st.st_size > (off_t)PKG_TEXT_MAX_SIZE)
    {
      close(fd);
      return -EFBIG;
    }

  length = (size_t)st.st_size;
  data = pkg_malloc((size_t)length + 1);
  if (data == NULL)
    {
      close(fd);
      return -ENOMEM;
    }

  total = 0;
  while (total < length)
    {
      ssize_t ret;

      ret = read(fd, data + total, length - total);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          close(fd);
          pkg_free(data);
          return -errno;
        }

      if (ret == 0)
        {
          break;
        }

      total += (size_t)ret;
    }

  nread = total;
  close(fd);

  if (nread != length)
    {
      pkg_free(data);
      return -EINVAL;
    }

  data[length] = '\0';
  *buffer = data;
  return 0;
}

#ifndef CONFIG_PSEUDOFS_FILE
/****************************************************************************
 * Name: pkg_store_make_tmp_path
 *
 * Description:
 *   Build a FAT short-name-compatible staging path.
 *
 ****************************************************************************/

static int pkg_store_make_tmp_path(FAR char *tmp, size_t size,
                                   FAR const char *path)
{
  FAR char *dot;
  FAR char *slash;
  int ret;

  ret = snprintf(tmp, size, "%s", path);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= size)
    {
      return -ENAMETOOLONG;
    }

  slash = strrchr(tmp, '/');
  dot = strrchr(slash != NULL ? slash : tmp, '.');
  if (dot != NULL)
    {
      *dot = '\0';
    }

  if (strlcat(tmp, ".tm", size) >= size)
    {
      return -ENAMETOOLONG;
    }

  return 0;
}
#endif

int pkg_store_write_text_atomic(FAR const char *path, FAR const char *text)
{
#ifdef CONFIG_PSEUDOFS_FILE
  int fd;
  int ret;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      return -errno;
    }

  ret = pkg_store_write_all(fd, text, strlen(text));
  if (ret < 0)
    {
      close(fd);
      unlink(path);
      return ret;
    }

  ret = close(fd);
  if (ret < 0)
    {
      ret = -errno;
      unlink(path);
      return ret;
    }

  return 0;
#else
  char tmp[PATH_MAX];
  int fd;
  int ret;

  ret = pkg_store_make_tmp_path(tmp, sizeof(tmp), path);
  if (ret < 0)
    {
      return ret;
    }

  fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      return -errno;
    }

  ret = pkg_store_write_all(fd, text, strlen(text));
  if (ret < 0)
    {
      close(fd);
      unlink(tmp);
      return ret;
    }

  /* Commit the temporary file before the atomic rename. */

  ret = fsync(fd);
  if (ret < 0)
    {
      ret = -errno;
      close(fd);
      unlink(tmp);
      return ret;
    }

  ret = close(fd);
  if (ret < 0)
    {
      ret = -errno;
      unlink(tmp);
      return ret;
    }

  ret = rename(tmp, path);
  if (ret < 0)
    {
      ret = -errno;
      unlink(tmp);
      return ret;
    }

  return 0;
#endif
}

int pkg_store_copy_file(FAR const char *src, FAR const char *dest)
{
  int infd;
  int outfd;
  int ret;
  char buffer[512];
#ifndef CONFIG_PSEUDOFS_FILE
  char tmp[PATH_MAX];
  FAR const char *outpath;

  ret = pkg_store_make_tmp_path(tmp, sizeof(tmp), dest);
  if (ret < 0)
    {
      return ret;
    }

  outpath = tmp;
#else
  FAR const char *outpath = dest;
#endif

  infd = open(src, O_RDONLY);
  if (infd < 0)
    {
      return -errno;
    }

  outfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (outfd < 0)
    {
      ret = -errno;
      close(infd);
      return ret;
    }

  for (; ; )
    {
      ssize_t nread;

      nread = read(infd, buffer, sizeof(buffer));
      if (nread < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          ret = -errno;
          goto errout;
        }

      if (nread == 0)
        {
          break;
        }

      ret = pkg_store_write_all(outfd, buffer, (size_t)nread);
      if (ret < 0)
        {
          goto errout;
        }
    }

  close(infd);

#ifndef CONFIG_PSEUDOFS_FILE
  /* Commit the payload before the atomic rename. */

  if (fsync(outfd) < 0)
    {
      ret = -errno;
      close(outfd);
      unlink(outpath);
      return ret;
    }
#endif

  ret = close(outfd);
  if (ret < 0)
    {
      ret = -errno;
      unlink(outpath);
      return ret;
    }

#ifndef CONFIG_PSEUDOFS_FILE
  if (rename(outpath, dest) < 0)
    {
      ret = -errno;
      unlink(outpath);
      return ret;
    }
#endif

  return 0;

errout:
  close(infd);
  close(outfd);
  unlink(outpath);
  return ret;
}

int pkg_store_remove_file(FAR const char *path)
{
  if (unlink(path) < 0)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  return 0;
}

int pkg_store_remove_version_dir(FAR const char *name,
                                 FAR const char *version)
{
  char path[PATH_MAX];
  char entry_path[PATH_MAX];
  FAR DIR *dir;
  FAR struct dirent *ent;
  int ret;

  /* Remove every file in a staged or installed version directory. */

  ret = pkg_store_format_version_path(path, sizeof(path), name, version);
  if (ret < 0)
    {
      return ret;
    }

  dir = opendir(path);
  if (dir == NULL)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  while ((ent = readdir(dir)) != NULL)
    {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
          continue;
        }

      ret = snprintf(entry_path, sizeof(entry_path), "%s/%s", path,
                     ent->d_name);
      if (ret > 0 && (size_t)ret < sizeof(entry_path))
        {
          unlink(entry_path);
        }
    }

  closedir(dir);

  if (rmdir(path) < 0)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  return 0;
}
