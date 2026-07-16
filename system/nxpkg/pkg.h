/****************************************************************************
 * apps/system/nxpkg/pkg.h
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

#ifndef __APPS_SYSTEM_NXPKG_PKG_H
#define __APPS_SYSTEM_NXPKG_PKG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <nuttx/kmalloc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PKG_ROOT_DIR          CONFIG_SYSTEM_NXPKG_ROOT
#define PKG_REPO_DIR          PKG_ROOT_DIR
#define PKG_REPO_INDEX        PKG_ROOT_DIR "/index.jsn"
#define PKG_REPO_SOURCE       PKG_ROOT_DIR "/repo.url"
#define PKG_REPO_INSTALLED    PKG_ROOT_DIR "/instpkg.jsn"
#define PKG_STORE_DIR         PKG_ROOT_DIR "/pkgs"
#define PKG_TMP_DIR           PKG_ROOT_DIR "/tmp"
#define PKG_TMP_PKG_DIR       PKG_ROOT_DIR "/tmp/pkg"

#define PKG_NAME_MAX          63
#define PKG_VERSION_MAX       31
#define PKG_ARCH_MAX          31
#define PKG_COMPAT_MAX        63
#define PKG_DESCRIPTION_MAX   127
#define PKG_CATEGORY_MAX      31
#define PKG_HASH_HEX_LEN      64
/* Each manifest slot is ~1.7KB (dominated by PKG_LAUNCH_ARGS_MAX slots).
 * This used to be 6 to fit inside a static internal-DRAM array, but the
 * catalog outgrew that once it held every real GSoC package (calc, the
 * four games, NXDoom, and the original examples) - system/nxstore/
 * nxstore_main.c now heap-allocates its struct pkg_index_s g_index via
 * pkg_zalloc(), which draws from the PSRAM-backed user heap on this board
 * (CONFIG_ESP32S3_SPIRAM_USER_HEAP) instead of internal DRAM, so this can
 * grow without the same pressure.  Still not unbounded: keep it sized for
 * "a real catalog", not arbitrary attacker-controlled growth.
 */

#define PKG_INDEX_MAX         16
#define PKG_INSTALLED_MAX     16
#define PKG_INSTALLED_VERSIONS_MAX 8
#define PKG_LAUNCH_ARGS_MAX   8
#define PKG_LAUNCH_ARG_MAX    127

/* Caps against a malicious/compromised HTTP server: without these, an
 * oversized response can exhaust SD-card space (downloads) or force an
 * unbounded single heap allocation sized directly off attacker-controlled
 * content (pkg_store_read_text).  Text/metadata files (index.jsn,
 * instpkg.jsn) are always small; artifact downloads cover the largest
 * real payloads seen in practice (a multi-MB WAD, a ~1MB game ELF) with
 * generous headroom.
 */

#define PKG_TEXT_MAX_SIZE     (256 * 1024)
#define PKG_DOWNLOAD_MAX_SIZE (32 * 1024 * 1024)

/* nxpkg is a one-shot CLI, not a daemon, so a lock file older than this
 * cannot belong to a still-running install under normal use (even a full
 * multi-MB artifact over a slow link finishes well within this window) -
 * it can only be left over from a process that was killed or a device
 * that lost power mid-install.  Reclaiming it is what makes install/
 * update/rollback usable again after the crash/power-loss scenarios this
 * target is prone to, instead of failing with EBUSY forever.
 */

#define PKG_LOCK_STALE_SECONDS (600)

static inline void *pkg_malloc(size_t size)
{
  void *ptr = malloc(size);

  if (ptr == NULL)
    {
      ptr = kmm_malloc(size);
    }

  return ptr;
}

static inline void *pkg_zalloc(size_t size)
{
  void *ptr = calloc(1, size);

  if (ptr == NULL)
    {
      ptr = kmm_zalloc(size);
    }

  return ptr;
}

static inline void *pkg_realloc(void *ptr, size_t size)
{
  if (ptr == NULL)
    {
      return pkg_malloc(size);
    }

  if (size == 0)
    {
      if (kmm_heapmember(ptr))
        {
          kmm_free(ptr);
        }
      else
        {
          free(ptr);
        }

      return NULL;
    }

  if (kmm_heapmember(ptr))
    {
      return kmm_realloc(ptr, size);
    }

  return realloc(ptr, size);
}

static inline void pkg_free(void *ptr)
{
  if (ptr == NULL)
    {
      return;
    }

  if (kmm_heapmember(ptr))
    {
      kmm_free(ptr);
    }
  else
    {
      free(ptr);
    }
}

static inline FAR char *pkg_path_alloc(void)
{
  return pkg_malloc(PATH_MAX);
}

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum pkg_payload_type_e
{
  PKG_PAYLOAD_ELF = 0,
  PKG_PAYLOAD_SHARED_LIB
};

enum pkg_txn_state_e
{
  PKG_TXN_IDLE = 0,
  PKG_TXN_FETCHING,
  PKG_TXN_VERIFIED,
  PKG_TXN_STAGED,
  PKG_TXN_COMPAT_OK,
  PKG_TXN_ACTIVATED,
  PKG_TXN_CLEANUP,
  PKG_TXN_FAILED,
  PKG_TXN_RESTORE
};

struct pkg_manifest_s
{
  char name[PKG_NAME_MAX + 1];
  char version[PKG_VERSION_MAX + 1];
  char arch[PKG_ARCH_MAX + 1];
  char compat[PKG_COMPAT_MAX + 1];
  char artifact[PATH_MAX];
  char sha256[PKG_HASH_HEX_LEN + 1];
  char launch_args[PKG_LAUNCH_ARGS_MAX][PKG_LAUNCH_ARG_MAX + 1];
  char description[PKG_DESCRIPTION_MAX + 1];
  char category[PKG_CATEGORY_MAX + 1];
  char icon[PATH_MAX];
  enum pkg_payload_type_e type;
  size_t launch_argc;
};

struct pkg_index_s
{
  struct pkg_manifest_s manifests[PKG_INDEX_MAX];
  size_t count;
};

struct pkg_installed_entry_s
{
  char name[PKG_NAME_MAX + 1];
  char current[PKG_VERSION_MAX + 1];
  char previous[PKG_VERSION_MAX + 1];
  char arch[PKG_ARCH_MAX + 1];
  char compat[PKG_COMPAT_MAX + 1];
  char versions[PKG_INSTALLED_VERSIONS_MAX][PKG_VERSION_MAX + 1];
  enum pkg_payload_type_e type;
  size_t version_count;
};

struct pkg_installed_db_s
{
  struct pkg_installed_entry_s entries[PKG_INSTALLED_MAX];
  size_t count;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

const char *pkg_manifest_type_str(enum pkg_payload_type_e type);
int pkg_manifest_validate(FAR const struct pkg_manifest_s *manifest);
int pkg_manifest_parse_type(FAR const char *value,
                            FAR enum pkg_payload_type_e *type);

int pkg_store_prepare_layout(void);
int pkg_store_ensure_package_root(FAR const char *name);
int pkg_store_ensure_version_dir(FAR const char *name,
                                 FAR const char *version);
int pkg_store_format_index_path(FAR char *buffer, size_t size);
int pkg_store_format_repo_source_path(FAR char *buffer, size_t size);
int pkg_store_format_installed_path(FAR char *buffer, size_t size);
int pkg_store_format_package_root(FAR char *buffer, size_t size,
                                  FAR const char *name);
int pkg_store_format_version_path(FAR char *buffer, size_t size,
                                  FAR const char *name,
                                  FAR const char *version);
int pkg_store_format_current_path(FAR char *buffer, size_t size,
                                  FAR const char *name);
int pkg_store_format_previous_path(FAR char *buffer, size_t size,
                                   FAR const char *name);
int pkg_store_format_txn_path(FAR char *buffer, size_t size,
                              FAR const char *name);
int pkg_store_format_lock_path(FAR char *buffer, size_t size,
                               FAR const char *name);
int pkg_store_format_download_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version);
int pkg_store_format_payload_path(FAR char *buffer, size_t size,
                                  FAR const char *name,
                                  FAR const char *version,
                                  FAR const char *artifact);
int pkg_store_format_manifest_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version);
int pkg_store_read_text(FAR const char *path, FAR char **buffer);
int pkg_store_write_text_atomic(FAR const char *path, FAR const char *text);
int pkg_store_copy_file(FAR const char *src, FAR const char *dest);
int pkg_store_remove_file(FAR const char *path);
int pkg_store_remove_version_dir(FAR const char *name,
                                 FAR const char *version);

const char *pkg_runtime_arch(void);
const char *pkg_runtime_compat(void);
int pkg_compat_check(FAR const struct pkg_manifest_s *manifest);

int pkg_hash_file_sha256(FAR const char *path,
                         FAR char digest[PKG_HASH_HEX_LEN + 1]);

int pkg_metadata_load_index_path(FAR const char *path,
                                 FAR struct pkg_index_s *index);
int pkg_metadata_load_index(FAR struct pkg_index_s *index);
FAR const struct pkg_manifest_s *
pkg_metadata_find_latest(FAR const struct pkg_index_s *index,
                         FAR const char *name);
int pkg_metadata_load_installed(FAR struct pkg_installed_db_s *db);
int pkg_metadata_save_installed(FAR const struct pkg_installed_db_s *db);
FAR struct pkg_installed_entry_s *
pkg_metadata_find_installed(FAR struct pkg_installed_db_s *db,
                            FAR const char *name);
int pkg_metadata_write_manifest(FAR const char *path,
                                FAR const struct pkg_manifest_s *manifest);
int pkg_metadata_print_installed(FAR FILE *stream,
                                 FAR const struct pkg_installed_db_s *db);

const char *pkg_txn_state_str(enum pkg_txn_state_e state);
int pkg_txn_write_state(FAR const char *name, enum pkg_txn_state_e state);
int pkg_txn_clear_state(FAR const char *name);

bool pkg_source_is_url(FAR const char *source);
int pkg_resolve_artifact_source(FAR char *buffer, size_t size,
                                FAR const struct pkg_manifest_s *manifest);
int pkg_resolve_icon_source(FAR char *buffer, size_t size,
                            FAR const struct pkg_manifest_s *manifest);
int pkg_acquire_source(FAR const char *source, FAR const char *dest);
int pkg_sync(FAR const char *source);
int pkg_install(FAR const char *name);
int pkg_uninstall(FAR const char *name);
int pkg_rollback(FAR const char *name);
int pkg_available(FAR FILE *stream);
int pkg_list(FAR FILE *stream);

void pkg_error(FAR const char *fmt, ...);
void pkg_info(FAR const char *fmt, ...);

#endif /* __APPS_SYSTEM_NXPKG_PKG_H */
