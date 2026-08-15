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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkg_install_acquire_lock(FAR const char *name, FAR char *path,
                                    size_t size)
{
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

  ret = pkg_lock_create(path);
  if (ret == -EEXIST)
    {
      pkg_reclaim_stale_lock(path);
      ret = pkg_lock_create(path);
    }

  return ret == -EEXIST ? -EBUSY : ret;
}

/****************************************************************************
 * Name: pkg_install_acquire_installed_lock
 *
 * Description:
 *   Serialize installed database updates.
 *
 ****************************************************************************/

static int pkg_install_acquire_installed_lock(FAR char *path, size_t size)
{
  int ret;
  int tries;

  ret = snprintf(path, size, PKG_ROOT_DIR "/instpkg.lk");
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= size)
    {
      return -ENAMETOOLONG;
    }

  for (tries = 0; tries < 100; tries++)
    {
      ret = pkg_lock_create(path);
      if (ret == 0)
        {
          return 0;
        }

      if (ret != -EEXIST)
        {
          return ret;
        }

      pkg_reclaim_stale_lock(path);
      usleep(20 * 1000);
    }

  return -EBUSY;
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

static int pkg_install_prune_oldest_version(
              FAR struct pkg_installed_entry_s *entry,
              FAR char *pruned_version, size_t pruned_version_size)
{
  size_t victim = entry->version_count;
  size_t i;

  /* Keep the current and rollback versions when pruning. */

  for (i = 0; i < entry->version_count; i++)
    {
      if (strcmp(entry->versions[i], entry->current) != 0 &&
          strcmp(entry->versions[i], entry->previous) != 0)
        {
          victim = i;
          break;
        }
    }

  if (victim == entry->version_count)
    {
      return -E2BIG;
    }

  /* Let the caller delete this version after committing the database. */

  snprintf(pruned_version, pruned_version_size, "%s",
           entry->versions[victim]);

  for (i = victim; i + 1 < entry->version_count; i++)
    {
      memcpy(entry->versions[i], entry->versions[i + 1],
             sizeof(entry->versions[i]));
    }

  entry->version_count--;
  return 0;
}

static int pkg_install_add_version(FAR struct pkg_installed_entry_s *entry,
                                   FAR const char *version,
                                   FAR char *pruned_version,
                                   size_t pruned_version_size)
{
  int ret;

  if (pkg_install_has_version(entry, version))
    {
      return 0;
    }

  if (entry->version_count >= PKG_INSTALLED_VERSIONS_MAX)
    {
      ret = pkg_install_prune_oldest_version(entry, pruned_version,
                                             pruned_version_size);
      if (ret < 0)
        {
          return ret;
        }
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
                                          *manifest,
                                        FAR char *pruned_version,
                                        size_t pruned_version_size)
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
  return pkg_install_add_version(entry, manifest->version, pruned_version,
                                 pruned_version_size);
}

static int pkg_install_write_pointers(
              FAR const struct pkg_installed_db_s *db,
              FAR const char *name)
{
  FAR struct pkg_installed_entry_s *entry;
  char current[PATH_MAX];
  char previous[PATH_MAX];
  int ret;

  entry = pkg_metadata_find_installed((FAR struct pkg_installed_db_s *)db,
                                      name);
  if (entry == NULL)
    {
      return -ENOENT;
    }

  ret = pkg_store_format_current_path(current, PATH_MAX, name);
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_format_previous_path(previous, PATH_MAX, name);
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
  FAR char *source;
  FAR char *tmp;
  FAR char *payload;
  FAR char *manifest_path;
  FAR char *lock;
  FAR char *installed_lock;
  FAR const char *artifact;
  char digest[PKG_HASH_HEX_LEN + 1];
  char pruned_version[PKG_VERSION_MAX + 1];
  bool staged_to_tmp;
  bool version_dir_created;
  bool installed_lock_held;
  int ret;

  pruned_version[0] = '\0';

  index = pkg_zalloc(sizeof(*index));
  installed = pkg_zalloc(sizeof(*installed));
  source = pkg_malloc(PATH_MAX);
  tmp = pkg_malloc(PATH_MAX);
  payload = pkg_malloc(PATH_MAX);
  manifest_path = pkg_malloc(PATH_MAX);
  lock = pkg_malloc(PATH_MAX);
  installed_lock = pkg_malloc(PATH_MAX);
  if (index == NULL || installed == NULL || source == NULL || tmp == NULL ||
      payload == NULL || manifest_path == NULL || lock == NULL ||
      installed_lock == NULL)
    {
      pkg_error("unable to allocate package metadata buffers");
      goto errout_early;
    }

  source[0] = '\0';
  tmp[0] = '\0';
  payload[0] = '\0';
  manifest_path[0] = '\0';
  lock[0] = '\0';
  installed_lock[0] = '\0';
  installed_lock_held = false;
  artifact = NULL;
  staged_to_tmp = false;
  version_dir_created = false;

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_error("unable to prepare package layout: %d", ret);
      goto errout_early;
    }

  ret = pkg_metadata_load_index(index);
  if (ret < 0)
    {
      pkg_error("unable to load local index metadata: %d", ret);
      goto errout_early;
    }

  manifest = pkg_metadata_find_latest(index, name);
  if (manifest == NULL)
    {
      pkg_error("package '%s' not found in local index", name);
      goto errout_early;
    }

  ret = pkg_resolve_artifact_source(source, PATH_MAX, manifest);
  if (ret < 0)
    {
      pkg_error("unable to resolve artifact source for '%s': %d", name, ret);
      goto errout_early;
    }

  ret = pkg_install_acquire_lock(name, lock, PATH_MAX);
  if (ret < 0)
    {
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      goto errout_early;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_FETCHING);
  if (ret < 0)
    {
      pkg_error("txn state fetching failed: %d", ret);
      goto errout;
    }

  if (pkg_source_is_url(source))
    {
      ret = pkg_store_format_download_path(tmp, PATH_MAX, manifest->name,
                                           manifest->version);
      if (ret < 0)
        {
          pkg_error("download path format failed: %d", ret);
          goto errout;
        }

      ret = pkg_acquire_source(source, tmp);
      if (ret < 0)
        {
          pkg_error("acquire source failed: %d", ret);
          goto errout;
        }

      artifact = tmp;
      staged_to_tmp = true;
    }
  else
    {
      artifact = source;
    }

  ret = pkg_hash_file_sha256(artifact, digest);
  if (ret < 0)
    {
      pkg_error("sha256 failed: %d", ret);
      goto errout;
    }

  if (strcasecmp(digest, manifest->sha256) != 0)
    {
      ret = -EILSEQ;
      pkg_error("sha256 mismatch: %d", ret);
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_VERIFIED);
  if (ret < 0)
    {
      pkg_error("txn state verified failed: %d", ret);
      goto errout;
    }

  ret = pkg_store_format_version_path(payload, PATH_MAX, manifest->name,
                                      manifest->version);
  if (ret < 0)
    {
      pkg_error("version path format failed: %d", ret);
      goto errout;
    }

  if (access(payload, F_OK) == 0)
    {
      version_dir_created = false;
    }
  else if (errno == ENOENT)
    {
      version_dir_created = true;
    }
  else
    {
      ret = -errno;
      pkg_error("unable to inspect version directory: %d", ret);
      goto errout;
    }

  ret = pkg_store_ensure_version_dir(manifest->name, manifest->version);
  if (ret < 0)
    {
      pkg_error("ensure version dir failed: %d", ret);
      goto errout;
    }

  ret = pkg_store_format_payload_path(payload, PATH_MAX,
                                      manifest->name, manifest->version,
                                      manifest->artifact);
  if (ret < 0)
    {
      pkg_error("payload path format failed: %d", ret);
      goto errout;
    }

  ret = pkg_store_copy_file(artifact, payload);
  if (ret < 0)
    {
      pkg_error("copy payload failed: %d", ret);
      goto errout;
    }

  if (manifest->type == PKG_PAYLOAD_ELF &&
      chmod(payload, 0755) < 0 && errno != ENOSYS)
    {
      ret = -errno;
      pkg_error("mark payload executable failed: %d", ret);
      goto errout;
    }

  ret = pkg_store_format_manifest_path(manifest_path, PATH_MAX,
                                       manifest->name, manifest->version);
  if (ret < 0)
    {
      pkg_error("manifest path format failed: %d", ret);
      goto errout;
    }

  ret = pkg_metadata_write_manifest(manifest_path, manifest);
  if (ret < 0)
    {
      pkg_error("write manifest failed: %d", ret);
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_STAGED);
  if (ret < 0)
    {
      pkg_error("txn state staged failed: %d", ret);
      goto errout;
    }

  ret = pkg_compat_check(manifest);
  if (ret < 0)
    {
      pkg_error("compat check failed: %d", ret);
      goto errout;
    }

  ret = pkg_txn_write_state(name, PKG_TXN_COMPAT_OK);
  if (ret < 0)
    {
      pkg_error("txn state compat_ok failed: %d", ret);
      goto errout;
    }

  ret = pkg_install_acquire_installed_lock(installed_lock, PATH_MAX);
  if (ret < 0)
    {
      pkg_error("unable to acquire installed-db lock: %d", ret);
      goto errout;
    }

  installed_lock_held = true;

  ret = pkg_metadata_load_installed(installed);
  if (ret < 0)
    {
      pkg_error("load installed metadata failed: %d", ret);
      goto errout;
    }

  ret = pkg_install_update_installed(installed, manifest, pruned_version,
                                     sizeof(pruned_version));
  if (ret < 0)
    {
      pkg_error("update installed metadata failed: %d", ret);
      goto errout;
    }

  ret = pkg_metadata_save_installed(installed);
  if (ret < 0)
    {
      pkg_error("save installed metadata failed: %d", ret);
      goto errout;
    }

  /* Remove the pruned payload after committing the database. */

  if (pruned_version[0] != '\0')
    {
      pkg_store_remove_version_dir(manifest->name, pruned_version);
    }

  ret = pkg_install_write_pointers(installed, manifest->name);
  if (ret < 0)
    {
      /* Pointer files can be rebuilt from the installed database. */

      pkg_error("unable to refresh current/previous pointers: %d", ret);
    }

  pkg_store_remove_file(installed_lock);
  installed_lock_held = false;

  ret = pkg_txn_write_state(name, PKG_TXN_ACTIVATED);
  if (ret < 0)
    {
      /* Do not remove payloads after the database commit. */

      pkg_error("txn state activated failed: %d", ret);
    }

  pkg_txn_write_state(name, PKG_TXN_CLEANUP);
  if (staged_to_tmp && tmp[0] != '\0')
    {
      pkg_store_remove_file(tmp);
    }

  pkg_txn_clear_state(name);
  if (lock[0] != '\0')
    {
      pkg_lock_remove(lock);
    }

  pkg_info("installed %s version %s", manifest->name, manifest->version);
  ret = EXIT_SUCCESS;
  goto freeout;

errout:
  pkg_txn_write_state(name, PKG_TXN_FAILED);
  if (staged_to_tmp && tmp[0] != '\0')
    {
      pkg_store_remove_file(tmp);
    }

  /* Remove a version directory created by this failed install. */

  if (version_dir_created)
    {
      pkg_store_remove_version_dir(manifest->name, manifest->version);
    }

  pkg_txn_clear_state(name);
  if (lock[0] != '\0')
    {
      pkg_lock_remove(lock);
    }

  if (installed_lock_held)
    {
      pkg_store_remove_file(installed_lock);
    }

  pkg_error("install failed for '%s': %d", name, ret);
  goto freeout;

errout_early:
  ret = EXIT_FAILURE;

freeout:
  pkg_free(index);
  pkg_free(installed);
  pkg_free(source);
  pkg_free(tmp);
  pkg_free(payload);
  pkg_free(manifest_path);
  pkg_free(lock);
  pkg_free(installed_lock);

  /* Preserve negative errno values for library callers. */

  return ret;
}

int pkg_list(FAR FILE *stream)
{
  FAR struct pkg_installed_db_s *db;
  int ret;

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      pkg_error("unable to allocate installed metadata buffer");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_free(db);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_free(db);
      pkg_error("unable to load installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_print_installed(stream, db);
  if (ret < 0)
    {
      pkg_free(db);
      pkg_error("unable to print installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  pkg_free(db);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: pkg_uninstall
 *
 * Description:
 *   Remove a package and all of its installed versions.
 *
 ****************************************************************************/

int pkg_uninstall(FAR const char *name)
{
  FAR struct pkg_installed_db_s *db;
  FAR struct pkg_installed_entry_s *entry;
  struct pkg_installed_entry_s removed;
  char path[PATH_MAX];
  char package_lock[PATH_MAX];
  char installed_lock[PATH_MAX];
  size_t index;
  size_t i;
  int ret;

  if (!pkg_validate_path_component(name))
    {
      pkg_error("remove requires a valid package name");
      return EXIT_FAILURE;
    }

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      pkg_error("unable to allocate installed metadata buffer");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_error("unable to prepare package layout: %d", ret);
      goto errout_with_db;
    }

  ret = pkg_install_acquire_lock(name, package_lock, sizeof(package_lock));
  if (ret < 0)
    {
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      goto errout_with_db;
    }

  ret = pkg_install_acquire_installed_lock(installed_lock,
                                           sizeof(installed_lock));
  if (ret < 0)
    {
      pkg_error("unable to acquire installed-db lock: %d", ret);
      goto errout_with_package_lock;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_error("unable to load installed metadata: %d", ret);
      goto errout_with_installed_lock;
    }

  entry = pkg_metadata_find_installed(db, name);
  if (entry == NULL)
    {
      pkg_error("package '%s' is not installed", name);
      goto errout_with_installed_lock;
    }

  /* Commit removal before deleting payloads. */

  removed = *entry;
  index = (size_t)(entry - db->entries);
  for (i = index; i + 1 < db->count; i++)
    {
      db->entries[i] = db->entries[i + 1];
    }

  db->count--;

  ret = pkg_metadata_save_installed(db);
  pkg_store_remove_file(installed_lock);
  if (ret < 0)
    {
      pkg_error("unable to save installed metadata: %d", ret);
      goto errout_with_package_lock;
    }

  for (i = 0; i < removed.version_count; i++)
    {
      pkg_store_remove_version_dir(name, removed.versions[i]);
    }

  if (pkg_store_format_txn_path(path, sizeof(path), name) == 0)
    {
      pkg_store_remove_file(path);
    }

  if (pkg_store_format_current_path(path, sizeof(path), name) == 0)
    {
      pkg_store_remove_file(path);
    }

  if (pkg_store_format_previous_path(path, sizeof(path), name) == 0)
    {
      pkg_store_remove_file(path);
    }

  pkg_store_remove_file(package_lock);

  if (pkg_store_format_package_root(path, sizeof(path), name) == 0)
    {
      rmdir(path);
    }

  pkg_info("removed %s", name);
  pkg_free(db);
  return EXIT_SUCCESS;

errout_with_installed_lock:
  pkg_store_remove_file(installed_lock);

errout_with_package_lock:
  pkg_store_remove_file(package_lock);

errout_with_db:
  pkg_free(db);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Name: pkg_rollback
 *
 * Description:
 *   Swap the current and previous installed versions.
 *
 ****************************************************************************/

int pkg_rollback(FAR const char *name)
{
  FAR struct pkg_installed_db_s *db;
  FAR struct pkg_installed_entry_s *entry;
  char version_path[PATH_MAX];
  char package_lock[PATH_MAX];
  char installed_lock[PATH_MAX];
  char swap[PKG_VERSION_MAX + 1];
  struct stat st;
  int ret;

  if (!pkg_validate_path_component(name))
    {
      pkg_error("rollback requires a valid package name");
      return EXIT_FAILURE;
    }

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      pkg_error("unable to allocate installed metadata buffer");
      return EXIT_FAILURE;
    }

  ret = pkg_store_prepare_layout();
  if (ret < 0)
    {
      pkg_error("unable to prepare package layout: %d", ret);
      goto errout_with_db;
    }

  ret = pkg_install_acquire_lock(name, package_lock, sizeof(package_lock));
  if (ret < 0)
    {
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      goto errout_with_db;
    }

  ret = pkg_install_acquire_installed_lock(installed_lock,
                                           sizeof(installed_lock));
  if (ret < 0)
    {
      pkg_error("unable to acquire installed-db lock: %d", ret);
      goto errout_with_package_lock;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_error("unable to load installed metadata: %d", ret);
      goto errout_with_installed_lock;
    }

  entry = pkg_metadata_find_installed(db, name);
  if (entry == NULL)
    {
      pkg_error("package '%s' is not installed", name);
      goto errout_with_installed_lock;
    }

  if (entry->previous[0] == '\0')
    {
      pkg_error("package '%s' has no previous version to roll back to",
                name);
      goto errout_with_installed_lock;
    }

  ret = pkg_store_format_version_path(version_path, sizeof(version_path),
                                      name, entry->previous);
  if (ret < 0 || stat(version_path, &st) < 0)
    {
      pkg_error("rollback target version '%s' is missing on disk",
                entry->previous);
      goto errout_with_installed_lock;
    }

  ret = snprintf(swap, sizeof(swap), "%s", entry->current);
  if (ret < 0 || (size_t)ret >= sizeof(swap))
    {
      pkg_error("current version string too long to swap");
      goto errout_with_installed_lock;
    }

  ret = snprintf(entry->current, sizeof(entry->current), "%s",
                 entry->previous);
  if (ret < 0 || (size_t)ret >= sizeof(entry->current))
    {
      pkg_error("unable to update current version");
      goto errout_with_installed_lock;
    }

  ret = snprintf(entry->previous, sizeof(entry->previous), "%s", swap);
  if (ret < 0 || (size_t)ret >= sizeof(entry->previous))
    {
      pkg_error("unable to update previous version");
      goto errout_with_installed_lock;
    }

  /* Commit the database before refreshing pointer files. */

  ret = pkg_metadata_save_installed(db);
  if (ret < 0)
    {
      pkg_error("unable to save installed metadata: %d", ret);
      goto errout_with_installed_lock;
    }

  ret = pkg_install_write_pointers(db, name);
  pkg_store_remove_file(installed_lock);
  pkg_store_remove_file(package_lock);
  if (ret < 0)
    {
      pkg_error("unable to refresh current/previous pointers: %d", ret);
    }

  pkg_info("rolled back %s to version %s", name, entry->current);
  pkg_free(db);
  return EXIT_SUCCESS;

errout_with_installed_lock:
  pkg_store_remove_file(installed_lock);

errout_with_package_lock:
  pkg_store_remove_file(package_lock);

errout_with_db:
  pkg_free(db);
  return EXIT_FAILURE;
}
