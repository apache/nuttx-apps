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
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pkg_install_reclaim_stale_lock
 *
 * Description:
 *   Remove "path" if it is old enough that it cannot belong to a
 *   still-running install (see PKG_LOCK_STALE_SECONDS).  Best-effort: any
 *   stat()/unlink() failure just falls through to the normal -EBUSY
 *   result, since a lock we can't inspect should be treated as held.
 *
 ****************************************************************************/

static void pkg_install_reclaim_stale_lock(FAR const char *path)
{
  struct stat st;
  time_t now;

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

  pkg_error("reclaiming stale lock '%s' (age %ld s)",
            path, (long)(now - st.st_mtime));
  unlink(path);
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
  if (fd < 0 && errno == EEXIST)
    {
      pkg_install_reclaim_stale_lock(path);
      fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    }

  if (fd < 0)
    {
      return errno == EEXIST ? -EBUSY : -errno;
    }

  close(fd);
  return 0;
}

/****************************************************************************
 * Name: pkg_install_acquire_installed_lock
 *
 * Description:
 *   The per-package lock above (pkg_install_acquire_lock()) only ever
 *   protects one package's own version directory/manifest - it says
 *   nothing about the single shared installed-packages database
 *   (instpkg.jsn) that every install/uninstall/rollback reads, modifies
 *   in memory, and writes back as a whole.  Two of those operations for
 *   *different* packages (their own per-package locks don't conflict)
 *   can each load a snapshot of that shared file, add/change their own
 *   entry, and save - and whichever one saves last wins, silently
 *   discarding whatever the other one had just added.  This is exactly
 *   what "a previously-installed package vanishes from `nxpkg list`
 *   with no error" looks like, without needing any corrupted file or
 *   non-atomic write at all: the write path is already atomic
 *   (pkg_store_write_text_atomic() writes to a temp file, fsyncs, then
 *   renames), so the *file* is always internally consistent - it just
 *   might be a consistent snapshot that's missing an entry a concurrent
 *   operation already believed it had durably saved.
 *
 *   A separate, single global lock file (independent of any specific
 *   package's own lock) closes that window: acquire it right before
 *   pkg_metadata_load_installed() and hold it until immediately after
 *   pkg_metadata_save_installed(), so that whole read-modify-write
 *   sequence is atomic with respect to every other caller of this
 *   function, regardless of which package(s) they're touching.
 *
 *   Blocking (bounded retry loop) rather than instant-EBUSY like the
 *   per-package lock: the critical section this protects is a handful
 *   of local SD-card JSON operations with no network I/O in it, so a
 *   real holder releases it within milliseconds - a caller should wait
 *   that out rather than fail an otherwise-healthy install/uninstall/
 *   rollback just because another one's shared-db update was in
 *   flight at the same instant.
 *
 ****************************************************************************/

static int pkg_install_acquire_installed_lock(FAR char *path, size_t size)
{
  int fd;
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
      fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
      if (fd >= 0)
        {
          close(fd);
          return 0;
        }

      if (errno != EEXIST)
        {
          return -errno;
        }

      pkg_install_reclaim_stale_lock(path);
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
              FAR struct pkg_installed_entry_s *entry)
{
  size_t victim = entry->version_count;
  size_t i;

  /* Versions are appended in install order, so the lowest index that
   * isn't the active ("current") or rollback ("previous") version is the
   * oldest one safe to drop.  Without this, a package updated more than
   * PKG_INSTALLED_VERSIONS_MAX times becomes permanently un-installable
   * (pkg_install_add_version would just fail forever).
   */

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

  pkg_store_remove_version_dir(entry->name, entry->versions[victim]);

  for (i = victim; i + 1 < entry->version_count; i++)
    {
      memcpy(entry->versions[i], entry->versions[i + 1],
            sizeof(entry->versions[i]));
    }

  entry->version_count--;
  return 0;
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
      ret = pkg_install_prune_oldest_version(entry);
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
      ret = -ENOENT;
      goto out;
    }

  ret = pkg_store_format_current_path(current, PATH_MAX, name);
  if (ret < 0)
    {
      goto out;
    }

  ret = pkg_store_format_previous_path(previous, PATH_MAX, name);
  if (ret < 0)
    {
      goto out;
    }

  ret = pkg_store_write_text_atomic(current, entry->current);
  if (ret < 0)
    {
      goto out;
    }

  ret = pkg_store_write_text_atomic(previous, entry->previous);

out:
  return ret;
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
  bool staged_to_tmp;
  bool version_dir_created;
  bool installed_lock_held;
  int ret;

  index = pkg_zalloc(sizeof(*index));
  installed = pkg_zalloc(sizeof(*installed));
  source = pkg_path_alloc();
  tmp = pkg_path_alloc();
  payload = pkg_path_alloc();
  manifest_path = pkg_path_alloc();
  lock = pkg_path_alloc();
  installed_lock = pkg_path_alloc();
  if (index == NULL || installed == NULL || source == NULL || tmp == NULL ||
      payload == NULL || manifest_path == NULL || lock == NULL ||
      installed_lock == NULL)
    {
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("unable to allocate package metadata buffers");
      return EXIT_FAILURE;
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
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_index(index);
  if (ret < 0)
    {
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("unable to load local index metadata: %d", ret);
      return EXIT_FAILURE;
    }

  manifest = pkg_metadata_find_latest(index, name);
  if (manifest == NULL)
    {
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("package '%s' not found in local index", name);
      return EXIT_FAILURE;
    }

  ret = pkg_resolve_artifact_source(source, PATH_MAX, manifest);
  if (ret < 0)
    {
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("unable to resolve artifact source for '%s': %d", name, ret);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_lock(name, lock, PATH_MAX);
  if (ret < 0)
    {
      pkg_free(index);
      pkg_free(installed);
      pkg_free(source);
      pkg_free(tmp);
      pkg_free(payload);
      pkg_free(manifest_path);
      pkg_free(lock);
      pkg_free(installed_lock);
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      return EXIT_FAILURE;
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

  ret = pkg_install_update_installed(installed, manifest);
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

  ret = pkg_install_write_pointers(installed, manifest->name);
  if (ret < 0)
    {
      /* The installed database is authoritative.  The pointer files are
       * convenience mirrors and can be reconstructed from it, so failure
       * to refresh one must not roll back a durably committed install.
       */

      pkg_error("unable to refresh current/previous pointers: %d", ret);
    }

  pkg_store_remove_file(installed_lock);
  installed_lock_held = false;

  ret = pkg_txn_write_state(name, PKG_TXN_ACTIVATED);
  if (ret < 0)
    {
      /* The installed database has already been committed.  Do not enter
       * the failure cleanup path here: it could remove a payload referenced
       * by that database.  Transaction state is recovery bookkeeping and
       * can be cleared below.
       */

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
      pkg_store_remove_file(lock);
    }

  pkg_info("installed %s version %s", manifest->name, manifest->version);
  pkg_free(index);
  pkg_free(installed);
  pkg_free(source);
  pkg_free(tmp);
  pkg_free(payload);
  pkg_free(manifest_path);
  pkg_free(lock);
  pkg_free(installed_lock);
  return EXIT_SUCCESS;

errout:
  pkg_txn_write_state(name, PKG_TXN_FAILED);
  if (staged_to_tmp && tmp[0] != '\0')
    {
      pkg_store_remove_file(tmp);
    }

  /* Reclaim whatever was staged into the version directory (payload,
   * manifest.jsn) before this failure - otherwise every failure past
   * this point leaves a permanently orphaned, never-activated version
   * directory with no way to reclaim it short of "remove".
   */

  if (version_dir_created)
    {
      pkg_store_remove_version_dir(manifest->name, manifest->version);
    }

  pkg_txn_clear_state(name);
  if (lock[0] != '\0')
    {
      pkg_store_remove_file(lock);
    }

  if (installed_lock_held)
    {
      pkg_store_remove_file(installed_lock);
    }

  pkg_free(index);
  pkg_free(installed);
  pkg_free(source);
  pkg_free(tmp);
  pkg_free(payload);
  pkg_free(manifest_path);
  pkg_free(lock);
  pkg_free(installed_lock);
  pkg_error("install failed for '%s': %d", name, ret);

  /* Propagate the real negative errno (rather than the constant
   * EXIT_FAILURE) here specifically, since every meaningful pipeline
   * failure - network/download, sha256 mismatch (-EILSEQ), wrong
   * arch/board (-ENOEXEC/-EXDEV from pkg_compat_check) - funnels through
   * this handler.  nxstore calls pkg_install() directly (not through a
   * shell) and can use this to show a differentiated message instead of
   * one generic "install failed" string; any nonzero value (this is
   * always < 0) still satisfies the plain success/failure contract for
   * callers that only check for zero.
   */

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
 *   Remove every installed version of "name": their version directories
 *   (payload + manifest.jsn), the current/previous pointer files, any
 *   leftover txn.tx, the entry in the shared installed-packages database,
 *   and finally the now-empty package root directory.  Refuses to run
 *   while an install/update for the same package is in flight (a live
 *   lock.lk), since removing the store out from under it would corrupt
 *   whatever it's mid-writing.
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
      pkg_free(db);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_lock(name, package_lock, sizeof(package_lock));
  if (ret < 0)
    {
      pkg_free(db);
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_installed_lock(installed_lock,
                                           sizeof(installed_lock));
  if (ret < 0)
    {
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to acquire installed-db lock: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to load installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  entry = pkg_metadata_find_installed(db, name);
  if (entry == NULL)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("package '%s' is not installed", name);
      return EXIT_FAILURE;
    }

  /* Drop this entry from the authoritative database before removing its
   * payloads.  A power loss can then leave reclaimable orphan files, but
   * never a database entry that points at a payload already deleted.
   */

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
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to save installed metadata: %d", ret);
      return EXIT_FAILURE;
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
}

/****************************************************************************
 * Name: pkg_rollback
 *
 * Description:
 *   Swap "name"'s current and previous installed versions.  The swap (as
 *   opposed to just clearing "previous") lets a second rollback undo the
 *   first.  Verifies the rollback target's version directory still
 *   exists on disk before committing any state change, and refuses to
 *   run while an install/update for the same package is in flight.
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
      pkg_free(db);
      pkg_error("unable to prepare package layout: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_lock(name, package_lock, sizeof(package_lock));
  if (ret < 0)
    {
      pkg_free(db);
      pkg_error("unable to acquire package lock for '%s': %d", name, ret);
      return EXIT_FAILURE;
    }

  ret = pkg_install_acquire_installed_lock(installed_lock,
                                           sizeof(installed_lock));
  if (ret < 0)
    {
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to acquire installed-db lock: %d", ret);
      return EXIT_FAILURE;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to load installed metadata: %d", ret);
      return EXIT_FAILURE;
    }

  entry = pkg_metadata_find_installed(db, name);
  if (entry == NULL)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("package '%s' is not installed", name);
      return EXIT_FAILURE;
    }

  if (entry->previous[0] == '\0')
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("package '%s' has no previous version to roll back to",
                name);
      return EXIT_FAILURE;
    }

  ret = pkg_store_format_version_path(version_path, sizeof(version_path),
                                      name, entry->previous);
  if (ret < 0 || stat(version_path, &st) < 0)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("rollback target version '%s' is missing on disk",
                entry->previous);
      return EXIT_FAILURE;
    }

  ret = snprintf(swap, sizeof(swap), "%s", entry->current);
  if (ret < 0 || (size_t)ret >= sizeof(swap))
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("current version string too long to swap");
      return EXIT_FAILURE;
    }

  ret = snprintf(entry->current, sizeof(entry->current), "%s",
                 entry->previous);
  if (ret < 0 || (size_t)ret >= sizeof(entry->current))
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to update current version");
      return EXIT_FAILURE;
    }

  ret = snprintf(entry->previous, sizeof(entry->previous), "%s", swap);
  if (ret < 0 || (size_t)ret >= sizeof(entry->previous))
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to update previous version");
      return EXIT_FAILURE;
    }

  /* The installed database is authoritative, so commit it first.  Refresh
   * the current/previous pointer files afterwards as convenience mirrors;
   * they can be reconstructed from the database if either write fails.
   */

  ret = pkg_metadata_save_installed(db);
  if (ret < 0)
    {
      pkg_store_remove_file(installed_lock);
      pkg_store_remove_file(package_lock);
      pkg_free(db);
      pkg_error("unable to save installed metadata: %d", ret);
      return EXIT_FAILURE;
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
}
