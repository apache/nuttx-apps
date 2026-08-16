/****************************************************************************
 * apps/include/system/nxpkg.h
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

#ifndef __APPS_INCLUDE_SYSTEM_NXPKG_H
#define __APPS_INCLUDE_SYSTEM_NXPKG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <limits.h>
#include <stddef.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PKG_ROOT_DIR               CONFIG_SYSTEM_NXPKG_ROOT
#define PKG_NAME_MAX               63
#define PKG_VERSION_MAX            31
#define PKG_ARCH_MAX               31
#define PKG_COMPAT_MAX             63
#define PKG_DESCRIPTION_MAX        127
#define PKG_CATEGORY_MAX           31
#define PKG_HASH_HEX_LEN           64
#define PKG_INDEX_MAX              16
#define PKG_INSTALLED_MAX          16
#define PKG_INSTALLED_VERSIONS_MAX 8
#define PKG_LAUNCH_ARGS_MAX        8
#define PKG_LAUNCH_ARG_MAX         127

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum pkg_payload_type_e
{
  PKG_PAYLOAD_ELF = 0,
  PKG_PAYLOAD_SHARED_LIB
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

int pkg_store_format_payload_path(FAR char *buffer, size_t size,
                                  FAR const char *name,
                                  FAR const char *version,
                                  FAR const char *artifact);
int pkg_store_format_manifest_path(FAR char *buffer, size_t size,
                                   FAR const char *name,
                                   FAR const char *version);
int pkg_metadata_load_index(FAR struct pkg_index_s *index);
int pkg_metadata_load_manifest_path(FAR const char *path,
                                    FAR struct pkg_manifest_s *manifest);
FAR const struct pkg_manifest_s *
pkg_metadata_find_latest(FAR const struct pkg_index_s *index,
                         FAR const char *name);
int pkg_metadata_load_installed(FAR struct pkg_installed_db_s *db);
FAR struct pkg_installed_entry_s *
pkg_metadata_find_installed(FAR struct pkg_installed_db_s *db,
                            FAR const char *name);
int pkg_resolve_icon_source(FAR char *buffer, size_t size,
                            FAR const struct pkg_manifest_s *manifest);
int pkg_acquire_source(FAR const char *source, FAR const char *dest);
int pkg_sync(FAR const char *source);
int pkg_install(FAR const char *name);
int pkg_uninstall(FAR const char *name);

#endif /* __APPS_INCLUDE_SYSTEM_NXPKG_H */
