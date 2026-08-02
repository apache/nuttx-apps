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
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
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

#define PKG_LOCK_RECORD_MAGIC "NXPKG1"
#define PKG_LOCK_RECORD_SIZE  64

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_once_t g_pkg_lock_boot_once = PTHREAD_ONCE_INIT;
static uint64_t g_pkg_lock_boot_id;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pkg_lock_init_boot_id(void)
{
  arc4random_buf(&g_pkg_lock_boot_id, sizeof(g_pkg_lock_boot_id));
  if (g_pkg_lock_boot_id == 0)
    {
      g_pkg_lock_boot_id = 1;
    }
}

static uint64_t pkg_lock_get_boot_id(void)
{
  if (pthread_once(&g_pkg_lock_boot_once, pkg_lock_init_boot_id) != 0)
    {
      return 1;
    }

  return g_pkg_lock_boot_id;
}

static int pkg_lock_read_owner(FAR const char *path,
                               FAR uint64_t *boot_id,
                               FAR pid_t *owner)
{
  char record[PKG_LOCK_RECORD_SIZE];
  unsigned long long parsed_boot;
  long parsed_owner;
  ssize_t nread;
  int fd;
  int ret;

  fd = open(path, O_RDONLY);
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

  ret = sscanf(record, PKG_LOCK_RECORD_MAGIC " %llx %ld",
               &parsed_boot, &parsed_owner);
  if (ret != 2 || parsed_owner <= 0 ||
      (long)(pid_t)parsed_owner != parsed_owner)
    {
      return -EINVAL;
    }

  *boot_id = (uint64_t)parsed_boot;
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
  char record[PKG_LOCK_RECORD_SIZE];
  int fd;
  int ret;

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd < 0)
    {
      return -errno;
    }

  ret = snprintf(record, sizeof(record), PKG_LOCK_RECORD_MAGIC
                 " %016" PRIx64 " %ld\n",
                 pkg_lock_get_boot_id(), (long)getpid());
  if (ret < 0 || (size_t)ret >= sizeof(record))
    {
      ret = ret < 0 ? ret : -ENAMETOOLONG;
      goto errout;
    }

  ret = pkg_store_write_all(fd, record, (size_t)ret);
  if (ret < 0)
    {
      goto errout;
    }

  if (fsync(fd) < 0)
    {
      ret = -errno;
      goto errout;
    }

  if (close(fd) < 0)
    {
      ret = -errno;
      unlink(path);
      return ret;
    }

  return 0;

errout:
  close(fd);
  unlink(path);
  return ret;
}

void pkg_reclaim_stale_lock(FAR const char *path)
{
  struct stat st;
  uint64_t boot_id;
  pid_t owner;
  time_t now;
  int ret;

  ret = pkg_lock_read_owner(path, &boot_id, &owner);
  if (ret == -EINVAL)
    {
      /* A creator may have completed open(O_EXCL) but not its first write.
       * Give that very small window time to close before treating the file
       * as a legacy timestamp-only lock.
       */

      usleep(20 * 1000);
      ret = pkg_lock_read_owner(path, &boot_id, &owner);
    }

  if (ret == 0)
    {
      if (boot_id != pkg_lock_get_boot_id())
        {
          pkg_error("reclaiming lock from an earlier boot '%s'", path);
          unlink(path);
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
          unlink(path);
        }

      return;
    }

  /* Compatibility for empty lock files created by older nxpkg images.
   * Their only ownership information is the filesystem timestamp.
   */

  if (stat(path, &st) < 0)
    {
      return;
    }

  now = time(NULL);
  if (now < st.st_mtime ||
      (now - st.st_mtime) < PKG_LOCK_STALE_SECONDS)
    {
      return;
    }

  pkg_error("reclaiming legacy stale lock '%s' (age %ld s)",
            path, (long)(now - st.st_mtime));
  unlink(path);
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

  /* This used to be "PKG_TMP_PKG_DIR/name-version.pkg", which breaks on
   * this SD card's short-name-only FAT mount as soon as name+version
   * exceeds the 8.3 8-character base-name limit - e.g. "nxdoom-1" (8
   * chars) fits and installs fine, but "nxdoom-10" or "nxdoom-9.1" (9+
   * chars) fails the open(O_CREAT) in pkg_repo_fetch_url() with
   * -EINVAL, surfacing as "acquire source failed: -22" for any
   * multi-character version - independent of name/version length here,
   * unlike pkg_store_make_tmp_path()'s already-FAT-safe scheme.  The
   * pid is small, bounded, and unique per concurrently running install
   * (each `nxpkg install` is its own process with its own per-name
   * lock), so it can't collide the way a single fixed name would if
   * two different packages were being installed at once.
   */

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

  /* Reject anything unreasonably large before the size is trusted for an
   * allocation: guards both against a malicious/oversized text file (this
   * path is used for the network-fetched index.jsn) and against
   * "length + 1" wrapping if st_size were ever attacker-influenced up to
   * SIZE_MAX.
   */

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
 *   Derive a staging path for an atomic write/copy to "path", under a
 *   short-name-compatible extension instead of appending ".tmp" (which
 *   would produce a second '.' in the final path component and break on
 *   FAT filesystems without long file name support).
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

  /* Force the write through to disk before renaming.  nxpkg writes
   * several small, unrelated files back-to-back during install (per-
   * package txn state, then the shared installed-packages database);
   * without an explicit sync here, the FAT driver's single shared
   * sector cache can still hold a not-yet-committed buffer for this
   * file when the very next atomic write starts touching a different
   * file, corrupting one or both.
   */

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
  /* Force the payload through to disk before renaming - this is the
   * largest write in the whole install pipeline (WAD/game-ELF-sized
   * payloads), so a hard power-loss here is the scenario the atomic
   * temp+rename is specifically protecting against.
   */

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

  /* Generic directory-content removal (rather than unlinking the payload
   * and manifest.jsn by their known names) so this same helper works both
   * to reclaim a partially staged version directory after a failed
   * install (pkg_install.c) and to prune/remove a fully-installed
   * version, without needing to already know that version's artifact
   * filename.  Best-effort throughout: this runs from error/cleanup
   * paths where a still-failing removal shouldn't itself abort the
   * caller.
   */

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
