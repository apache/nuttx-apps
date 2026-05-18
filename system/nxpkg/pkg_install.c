/****************************************************************************
 * apps/system/nxpkg/pkg_install.c
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
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkg_install_resolve_artifact(FAR char *buffer, size_t size,
                                        FAR const struct pkg_manifest_s
                                          *manifest)
{
  int ret;

  if (manifest->artifact[0] == '/')
    {
      ret = snprintf(buffer, size, "%s", manifest->artifact);
      if (ret < 0)
        {
          return ret;
        }

      return (size_t)ret >= size ? -ENAMETOOLONG : 0;
    }

  ret = snprintf(buffer, size, "%s/%s", PKG_REPO_DIR, manifest->artifact);
  if (ret < 0)
    {
      return ret;
    }

  return (size_t)ret >= size ? -ENAMETOOLONG : 0;
}

static int pkg_install_acquire_lock(FAR const char *name, FAR char *path,
                                    size_t size)
{
  int fd;
  int ret;

  ret = pkg_store_ensure_package_root(name);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_format_lock_path(path, size, name);
  if (ret < 0)
    {
      return ret;
    }

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd < 0)
    {
      return errno == EEXIST ? -EBUSY : -errno;
    }

  close(fd);
  return 0;
}

static bool pkg_install_has_version(
              FAR const struct pkg_installed_entry_s *entry,
              FAR const char *version)
{
  size_t i;

  for (i = 0; i < entry->version_count; i++)
    {
      if (strcmp(entry->versions[i], version) == 0)
        {
          return true;
        }
    }

  return false;
}

static int pkg_install_add_version(FAR struct pkg_installed_entry_s *entry,
                                   FAR const char *version)
{
  int ret;

  if (pkg_install_has_version(entry, version))
    {
      return 0;
    }

  if (entry->version_count >= PKG_INSTALLED_VERSIONS_MAX)
    {
      return -E2BIG;
    }

  ret = snprintf(entry->versions[entry->version_count],
                 sizeof(entry->versions[entry->version_count]),
                 "%s", version);
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= sizeof(entry->versions[entry->version_count]))
    {
      return -ENAMETOOLONG;
    }

  entry->version_count++;
  return 0;
}

static int pkg_install_update_installed(FAR struct pkg_installed_db_s *db,
                                        FAR const struct pkg_manifest_s
                                          *manifest)
{
  FAR struct pkg_installed_entry_s *entry;
  int ret;

  entry = pkg_metadata_find_installed(db, manifest->name);
  if (entry == NULL)
    {
      if (db->count >= PKG_INSTALLED_MAX)
        {
          return -E2BIG;
        }

      entry = &db->entries[db->count++];
      memset(entry, 0, sizeof(*entry));
      ret = snprintf(entry->name, sizeof(entry->name), "%s", manifest->name);
      if (ret < 0 || (size_t)ret >= sizeof(entry->name))
        {
          return -ENAMETOOLONG;
        }
    }

  if (entry->current[0] != '\0' &&
      strcmp(entry->current, manifest->version) != 0)
    {
      ret = snprintf(entry->previous, sizeof(entry->previous), "%s",
                     entry->current);
      if (ret < 0 || (size_t)ret >= sizeof(entry->previous))
        {
          return -ENAMETOOLONG;
        }
    }

  ret = snprintf(entry->current, sizeof(entry->current), "%s",
                 manifest->version);
  if (ret < 0 || (size_t)ret >= sizeof(entry->current))
    {
      return -ENAMETOOLONG;
    }

  ret = snprintf(entry->arch, sizeof(entry->arch), "%s", manifest->arch);
  if (ret < 0 || (size_t)ret >= sizeof(entry->arch))
    {
      return -ENAMETOOLONG;
    }

  ret = snprintf(entry->compat, sizeof(entry->compat), "%s",
                 manifest->compat);
  if (ret < 0 || (size_t)ret >= sizeof(entry->compat))
    {
      return -ENAMETOOLONG;
    }

  entry->type = manifest->type;
  return pkg_install_add_version(entry, manifest->version);
}

static int pkg_install_write_pointers(
              FAR const struct pkg_installed_db_s *db,
              FAR const struct pkg_manifest_s *manifest)
{
  FAR struct pkg_installed_entry_s *entry;
  char current[PATH_MAX];
  char previous[PATH_MAX];
  int ret;

  entry = pkg_metadata_find_installed((FAR struct pkg_installed_db_s *)db,
                                      manifest->name);
  if (entry == NULL)
    {
      return -ENOENT;
    }

  ret = pkg_store_format_current_path(current, sizeof(current),
                                      manifest->name);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_format_previous_path(previous, sizeof(previous),
                                       manifest->name);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_write_text_atomic(current, entry->current);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_store_write_text_atomic(previous, entry->previous);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int pkg_install(FAR const char *name)
{
  FAR struct pkg_index_s *index;
  FAR struct pkg_installed_db_s *installed;
  FAR const struct pkg_manifest_s *manifest;
  char source[PATH_MAX];
  char tmp[PATH_MAX] = "";
  char payload[PATH_MAX];
  char manifest_path[PATH_MAX];
  char lock[PATH_MAX] = "";
  char digest[PKG_HASH_HEX_LEN + 1];
  int ret;

  index = malloc(sizeof(*index));
  installed = malloc(sizeof(*installed));
  if (index == NULL || installed == NULL)
    {
      free(index);
      free(installed);
      pkg_error("unable to allocate package metadata buffers");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      free(index);
      free(installed);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_index(index);
  if (ret < 0)
    {
      free(index);
      free(installed);
      pkg_error("unable to load local index metadata: %d", ret);
      return EXIT_FAILURE;
    }

  manifest = pkg_metadata_find_latest(index, name);
  if (manifest == NULL)
    {
      free(index);
      free(installed);
      pkg_error("package '%s' not found in local index", name);
      return EXIT_FAILURE;
    }

  ret = pkg_install_resolve_artifact(source, sizeof(source), manifest);
  if (ret < 0)
    {
      pkg_error("artifact path for '%s' is too long", name);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_lock(name, lock, sizeof(lock));
  if (ret < 0)
    {
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      return EXIT_FAILURE;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_FETCHING);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_format_download_path(tmp, sizeof(tmp), manifest->name,
                                       manifest->version);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_copy_file(source, tmp);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_hash_file_sha256(tmp, digest);
  if (ret < 0)
    {
      goto errout;
    }

  if (strcasecmp(digest, manifest->sha256) != 0)
    {
      ret = -EILSEQ;
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_VERIFIED);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_ensure_version_dir(manifest->name, manifest->version);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_format_payload_path(payload, sizeof(payload),
                                      manifest->name, manifest->version,
                                      manifest->artifact);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_copy_file(tmp, payload);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_store_format_manifest_path(manifest_path, sizeof(manifest_path),
                                       manifest->name, manifest->version);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_metadata_write_manifest(manifest_path, manifest);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_STAGED);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_compat_check(manifest);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_COMPAT_OK);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_metadata_load_installed(installed);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_install_update_installed(installed, manifest);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_install_write_pointers(installed, manifest);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_metadata_save_installed(installed);
  if (ret < 0)
    {
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_ACTIVATED);
  if (ret < 0)
    {
      goto errout;
    }

  pkg_txn_write_state(name, PKG_TXN_CLEANUP);
  if (tmp[0] != '\0')
    {
      pkg_store_remove_file(tmp);
    }

  pkg_txn_clear_state(name);
  if (lock[0] != '\0')
    {
      pkg_store_remove_file(lock);
    }

  pkg_info("installed %s version %s", manifest->name, manifest->version);
  free(index);
  free(installed);
  return EXIT_SUCCESS;

errout:
  pkg_txn_write_state(name, PKG_TXN_FAILED);
  if (tmp[0] != '\0')
    {
      pkg_store_remove_file(tmp);
    }

  pkg_txn_clear_state(name);
  if (lock[0] != '\0')
    {
      pkg_store_remove_file(lock);
    }

  free(index);
  free(installed);
  pkg_error("install failed for '%s': %d", name, ret);
  return EXIT_FAILURE;
}

int pkg_list(FAR FILE *stream)
{
  FAR struct pkg_installed_db_s *db;
  int ret;

  db = malloc(sizeof(*db));
  if (db == NULL)
    {
      pkg_error("unable to allocate installed metadata buffer");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      free(db);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      free(db);
      pkg_error("unable to load installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_print_installed(stream, db);
  if (ret < 0)
    {
      free(db);
      pkg_error("unable to print installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  free(db);
  return EXIT_SUCCESS;
}
