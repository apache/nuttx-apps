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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/.txn", name, "");
}

int pkg_store_format_lock_path(FAR char *buffer, size_t size,
                               FAR const char *name)
{
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/.lock", name, "");
}

int pkg_store_format_download_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version)
{
  return pkg_store_format(buffer, size, PKG_TMP_PKG_DIR "/%s-%s.npkg", name,
                          version);
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
  return pkg_store_format(buffer, size, PKG_STORE_DIR "/%s/%s/manifest.json",
                          name, version);
}

int pkg_store_read_text(FAR const char *path, FAR char **buffer)
{
  FAR FILE *stream;
  FAR char *data;
  long length;
  size_t nread;

  if (buffer == NULL)
    {
      return -EINVAL;
    }

  *buffer = NULL;

  stream = fopen(path, "rb");
  if (stream == NULL)
    {
      return errno == ENOENT ? -ENOENT : -errno;
    }

  if (fseek(stream, 0, SEEK_END) < 0)
    {
      fclose(stream);
      return -errno;
    }

  length = ftell(stream);
  if (length < 0)
    {
      fclose(stream);
      return -errno;
    }

  if (fseek(stream, 0, SEEK_SET) < 0)
    {
      fclose(stream);
      return -errno;
    }

  data = malloc((size_t)length + 1);
  if (data == NULL)
    {
      fclose(stream);
      return -ENOMEM;
    }

  nread = fread(data, 1, (size_t)length, stream);
  if (nread != (size_t)length)
    {
      int err = ferror(stream);

      fclose(stream);
      free(data);
      return err ? -EIO : -EINVAL;
    }

  fclose(stream);

  data[length] = '\0';
  *buffer = data;
  return 0;
}

int pkg_store_write_text_atomic(FAR const char *path, FAR const char *text)
{
  char tmp[PATH_MAX];
  int fd;
  int ret;

  ret = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= sizeof(tmp))
    {
      return -ENAMETOOLONG;
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

  if (close(fd) < 0)
    {
      unlink(tmp);
      return -errno;
    }

  if (rename(tmp, path) < 0)
    {
      unlink(tmp);
      return -errno;
    }

  return 0;
}

int pkg_store_copy_file(FAR const char *src, FAR const char *dest)
{
  int infd;
  int outfd;
  int ret;
  char buffer[512];

  infd = open(src, O_RDONLY);
  if (infd < 0)
    {
      return -errno;
    }

  outfd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

  if (close(outfd) < 0)
    {
      unlink(dest);
      return -errno;
    }

  return 0;

errout:
  close(infd);
  close(outfd);
  unlink(dest);
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
