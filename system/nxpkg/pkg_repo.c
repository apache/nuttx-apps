/****************************************************************************
 * apps/system/nxpkg/pkg_repo.c
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <nuttx/config.h>

#ifdef CONFIG_NETUTILS_WEBCLIENT
#  include "netutils/webclient.h"
#endif

#include "pkg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Each webclient_perform() read/sink cycle costs a TCP receive plus an
 * SD-card write call; at the old 512-byte size, a sub-1MB file like
 * nxdoom's ~940KB ELF took ~1900 round trips and, in practice, close
 * to two minutes to install - easily read as "stuck" with only a bare
 * spinner for feedback.  4KB cuts that to ~230 round trips and lines
 * up with typical SD card erase-block granularity, which also reduces
 * write amplification.  Still small enough to be a safe stack-local
 * buffer against the 16KB (CLI) / 16KB (nxstore install worker) task
 * stacks that call into this.
 */

#define PKG_REPO_FETCH_BUFFER_SIZE 4096

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_WEBCLIENT
struct pkg_fetch_context_s
{
  int fd;
  size_t total;
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkg_repo_copy_string(FAR char *buffer, size_t size,
                                FAR const char *value)
{
  int ret;

  ret = snprintf(buffer, size, "%s", value);
  if (ret < 0)
    {
      return ret;
    }

  return (size_t)ret >= size ? -ENAMETOOLONG : 0;
}

static int pkg_repo_source_base(FAR char *buffer, size_t size,
                                FAR const char *source)
{
  FAR const char *slash;
  size_t length;

  slash = strrchr(source, '/');
  if (slash == NULL)
    {
      return -EINVAL;
    }

  length = (size_t)(slash - source);
  if (length == 0)
    {
      length = 1;
    }

  if (length >= size)
    {
      return -ENAMETOOLONG;
    }

  memcpy(buffer, source, length);
  buffer[length] = '\0';
  return 0;
}

/****************************************************************************
 * Name: pkg_validate_artifact_relative
 *
 * Description:
 *   manifest->artifact is repo-relative content and gets spliced into a
 *   local filesystem path (or used to build a URL) unsanitized. Reject
 *   absolute paths outright - allowing them let a malicious index turn
 *   any local file with a known/predictable hash into an "installed"
 *   package, including making it executable - and reject any ".." path
 *   segment that would let the artifact escape the repo mirror directory.
 *
 ****************************************************************************/

static bool pkg_validate_artifact_relative(FAR const char *value)
{
  FAR const char *p;

  if (value == NULL || value[0] == '\0' || value[0] == '/')
    {
      return false;
    }

  p = value;
  while ((p = strstr(p, "..")) != NULL)
    {
      bool at_start = p == value || *(p - 1) == '/';
      bool at_end = p[2] == '\0' || p[2] == '/';

      if (at_start && at_end)
        {
          return false;
        }

      p++;
    }

  return true;
}

static int pkg_repo_read_source(FAR char *buffer, size_t size)
{
  char path[PATH_MAX];
  FAR char *text;
  size_t length;
  int ret;

  ret = pkg_store_format_repo_source_path(path, sizeof(path));
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_read_text(path, &text);
  if (ret < 0)
    {
      return ret;
    }

  length = strlen(text);
  while (length > 0 && isspace((unsigned char)text[length - 1]))
    {
      text[--length] = '\0';
    }

  ret = pkg_repo_copy_string(buffer, size, text);
  pkg_free(text);
  return ret;
}

#ifdef CONFIG_NETUTILS_WEBCLIENT
static int pkg_repo_sink(FAR char **buffer, int offset, int datend,
                         FAR int *buflen, FAR void *arg)
{
  FAR struct pkg_fetch_context_s *ctx;
  size_t remaining;
  FAR char *cursor;

  UNUSED(buffer);
  UNUSED(buflen);

  ctx = arg;
  cursor = &(*buffer)[offset];
  remaining = (size_t)(datend - offset);

  /* Cap total downloaded bytes: an unbounded/malicious response could
   * otherwise exhaust all SD-card space.  Checked before writing more so
   * the on-disk file never exceeds the cap even mid-chunk.
   */

  if (remaining > 0 &&
      (ctx->total > PKG_DOWNLOAD_MAX_SIZE ||
       remaining > PKG_DOWNLOAD_MAX_SIZE - ctx->total))
    {
      return -EFBIG;
    }

  ctx->total += remaining;

  while (remaining > 0)
    {
      ssize_t nwritten;

      nwritten = write(ctx->fd, cursor, remaining);
      if (nwritten < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      if (nwritten == 0)
        {
          return -EIO;
        }

      cursor += nwritten;
      remaining -= (size_t)nwritten;
    }

  return 0;
}

static int pkg_repo_fetch_url(FAR const char *url, FAR const char *dest)
{
  struct pkg_fetch_context_s fetch;
  struct webclient_context client;
  char reason[64];
  char buffer[PKG_REPO_FETCH_BUFFER_SIZE];
  int ret;

  fetch.fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fetch.fd < 0)
    {
      return -errno;
    }

  fetch.total = 0;

  webclient_set_defaults(&client);
  client.method = "GET";
  client.url = url;
  client.buffer = buffer;
  client.buflen = sizeof(buffer);
  client.sink_callback = pkg_repo_sink;
  client.sink_callback_arg = &fetch;
  client.http_reason = reason;
  client.http_reason_len = sizeof(reason);

  ret = webclient_perform(&client);
  if (ret < 0)
    {
      close(fetch.fd);
      unlink(dest);
      return ret;
    }

  if (client.http_status / 100 != 2)
    {
      close(fetch.fd);
      unlink(dest);
      return -EPROTO;
    }

  if (close(fetch.fd) < 0)
    {
      unlink(dest);
      return -errno;
    }

  return 0;
}
#endif

static int pkg_resolve_relative_source(FAR char *buffer, size_t size,
                                       FAR const char *relative)
{
  char source[PATH_MAX];
  char base[PATH_MAX];
  int ret;

  if (buffer == NULL || relative == NULL || relative[0] == '\0')
    {
      return -EINVAL;
    }

  if (pkg_source_is_url(relative))
    {
      return pkg_repo_copy_string(buffer, size, relative);
    }

  if (!pkg_validate_artifact_relative(relative))
    {
      return -EINVAL;
    }

  ret = pkg_repo_read_source(source, sizeof(source));
  if (ret >= 0)
    {
      ret = pkg_repo_source_base(base, sizeof(base), source);
      if (ret < 0)
        {
          return ret;
        }

      ret = snprintf(buffer, size, "%s/%s", base, relative);
      if (ret < 0)
        {
          return ret;
        }

      return (size_t)ret >= size ? -ENAMETOOLONG : 0;
    }

  ret = snprintf(buffer, size, "%s/%s", PKG_REPO_DIR, relative);
  if (ret < 0)
    {
      return ret;
    }

  return (size_t)ret >= size ? -ENAMETOOLONG : 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool pkg_source_is_url(FAR const char *source)
{
  if (source == NULL)
    {
      return false;
    }

  return strncasecmp(source, "http://", 7) == 0 ||
         strncasecmp(source, "https://", 8) == 0;
}

int pkg_resolve_artifact_source(FAR char *buffer, size_t size,
                                FAR const struct pkg_manifest_s *manifest)
{
  if (manifest == NULL)
    {
      return -EINVAL;
    }

  return pkg_resolve_relative_source(buffer, size, manifest->artifact);
}

int pkg_resolve_icon_source(FAR char *buffer, size_t size,
                            FAR const struct pkg_manifest_s *manifest)
{
  if (manifest == NULL)
    {
      return -EINVAL;
    }

  return pkg_resolve_relative_source(buffer, size, manifest->icon);
}

int pkg_acquire_source(FAR const char *source, FAR const char *dest)
{
  if (source == NULL || dest == NULL)
    {
      return -EINVAL;
    }

  if (pkg_source_is_url(source))
    {
#ifdef CONFIG_NETUTILS_WEBCLIENT
      return pkg_repo_fetch_url(source, dest);
#else
      return -ENOSYS;
#endif
    }

  return pkg_store_copy_file(source, dest);
}

int pkg_sync(FAR const char *source)
{
  FAR struct pkg_index_s *index;
  FAR char *text = NULL;
  FAR char *tmp;
  FAR char *index_path;
  FAR char *source_path;
  int ret;

  if (source == NULL || source[0] == '\0')
    {
      pkg_error("sync requires a non-empty index source");
      return -EINVAL;
    }

  index = pkg_zalloc(sizeof(*index));
  tmp = pkg_path_alloc();
  index_path = pkg_path_alloc();
  source_path = pkg_path_alloc();
  if (index == NULL || tmp == NULL || index_path == NULL ||
      source_path == NULL)
    {
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      pkg_error("unable to allocate index metadata buffer");
      return -ENOMEM;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_error("unable to prepare package layout: %d", ret);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      return ret;
    }

  /* Each CLI invocation has its own PID.  Use it in the staging name so a
   * manual sync cannot truncate the file being validated by nxstore (or a
   * second shell).  Keep the leaf short for short-name-only FAT mounts.
   */

  ret = snprintf(tmp, PATH_MAX, "%s/s%u.jsn", PKG_TMP_DIR,
                 (unsigned int)getpid());
  if (ret < 0 || (size_t)ret >= PATH_MAX)
    {
      pkg_error("temporary sync path is too long");
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      return -ENAMETOOLONG;
    }

  ret = pkg_acquire_source(source, tmp);
  if (ret < 0)
    {
      pkg_error("unable to fetch index source '%s': %d", source, ret);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      return ret;
    }

  ret = pkg_metadata_load_index_path(tmp, index);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_error("downloaded index is invalid: %d", ret);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      return ret;
    }

  ret = pkg_store_read_text(tmp, &text);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_error("unable to read fetched index: %d", ret);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      return ret;
    }

  ret = pkg_store_format_index_path(index_path, PATH_MAX);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_free(text);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      pkg_error("unable to resolve local index path: %d", ret);
      return ret;
    }

  ret = pkg_store_write_text_atomic(index_path, text);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_free(text);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      pkg_error("unable to write local index: %d", ret);
      return ret;
    }

  ret = pkg_store_format_repo_source_path(source_path, PATH_MAX);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_free(text);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      pkg_error("unable to resolve repository source path: %d", ret);
      return ret;
    }

  ret = pkg_store_write_text_atomic(source_path, source);
  if (ret < 0)
    {
      pkg_store_remove_file(tmp);
      pkg_free(text);
      pkg_free(index);
      pkg_free(tmp);
      pkg_free(index_path);
      pkg_free(source_path);
      pkg_error("unable to save repository source: %d", ret);
      return ret;
    }

  pkg_store_remove_file(tmp);
  pkg_free(text);
  pkg_free(index);
  pkg_free(tmp);
  pkg_free(index_path);
  pkg_free(source_path);
  pkg_info("synced package index from %s", source);
  return 0;
}

int pkg_available(FAR FILE *stream)
{
  FAR struct pkg_index_s *index;
  FAR const char *arch;
  FAR const char *compat;
  size_t i;
  int ret;

  if (stream == NULL)
    {
      return EXIT_FAILURE;
    }

  index = pkg_zalloc(sizeof(*index));
  if (index == NULL)
    {
      pkg_error("unable to allocate index metadata buffer");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_free(index);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_index(index);
  if (ret < 0)
    {
      pkg_free(index);
      pkg_error("unable to load package index: %d", ret);
      return EXIT_FAILURE;
    }

  arch = pkg_runtime_arch();
  compat = pkg_runtime_compat();

  for (i = 0; i < index->count; i++)
    {
      FAR const struct pkg_manifest_s *manifest = &index->manifests[i];
      FAR const struct pkg_manifest_s *latest;

      if (strcmp(manifest->arch, arch) != 0 ||
          strcmp(manifest->compat, compat) != 0)
        {
          continue;
        }

      latest = pkg_metadata_find_latest(index, manifest->name);
      if (latest != manifest)
        {
          continue;
        }

      fprintf(stream,
              "%s version=%s type=%s arch=%s compat=%s artifact=%s\n",
              manifest->name,
              manifest->version,
              pkg_manifest_type_str(manifest->type),
              manifest->arch,
              manifest->compat,
              manifest->artifact);
    }

  pkg_free(index);
  return EXIT_SUCCESS;
}
