/****************************************************************************
 * apps/system/nxpkg/pkg_manifest.c
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
#include <string.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool pkg_has_nonspace(FAR const char *value)
{
  while (*value != '\0')
    {
      if (!isspace((unsigned char)*value))
        {
          return true;
        }

      value++;
    }

  return false;
}

static int pkg_validate_required(FAR const char *value)
{
  if (value == NULL || value[0] == '\0' || !pkg_has_nonspace(value))
    {
      return -EINVAL;
    }

  return 0;
}

static bool pkg_validate_hex(FAR const char *value)
{
  while (*value != '\0')
    {
      if (!isxdigit((unsigned char)*value))
        {
          return false;
        }

      value++;
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pkg_validate_path_component
 *
 * Description:
 *   Reject any value that could escape the intended directory when spliced
 *   into a filesystem path (pkg_store.c's PKG_STORE_DIR "/%s/%s/..."
 *   formatters). This is required for "name" and "version" specifically,
 *   since both come straight from an untrusted, network-fetched
 *   index.json and are used unsanitized to build install paths - a
 *   version of "../../evil" would otherwise let a malicious index write
 *   or delete files outside the package store entirely.
 *
 ****************************************************************************/

bool pkg_validate_path_component(FAR const char *value)
{
  FAR const char *p;

  if (pkg_validate_required(value) < 0)
    {
      return false;
    }

  /* Reject a leading '.' outright: blocks ".", "..", and any
   * "../"-prefixed traversal in one check.
   */

  if (value[0] == '.')
    {
      return false;
    }

  for (p = value; *p != '\0'; p++)
    {
      if (*p == '/' || *p == '\\')
        {
          return false;
        }
    }

  return true;
}

const char *pkg_manifest_type_str(enum pkg_payload_type_e type)
{
  switch (type)
    {
      case PKG_PAYLOAD_ELF:
        return "elf";

      case PKG_PAYLOAD_SHARED_LIB:
        return "shared-lib";

      default:
        return "unknown";
    }
}

int pkg_manifest_validate(FAR const struct pkg_manifest_s *manifest)
{
  size_t i;

  if (manifest == NULL)
    {
      return -EINVAL;
    }

  if (pkg_validate_required(manifest->name) < 0 ||
      pkg_validate_required(manifest->version) < 0 ||
      pkg_validate_required(manifest->arch) < 0 ||
      pkg_validate_required(manifest->compat) < 0 ||
      pkg_validate_required(manifest->artifact) < 0 ||
      pkg_validate_required(manifest->sha256) < 0)
    {
      return -EINVAL;
    }

  /* "name" and "version" get spliced unsanitized into on-disk paths
   * (pkg_store.c) - they must not contain path separators or traversal
   * sequences.  "artifact" is validated separately in pkg_repo.c, where
   * it's legitimately allowed to be a relative repo path (just not an
   * absolute one or one that escapes the repo root).
   */

  if (!pkg_validate_path_component(manifest->name) ||
      !pkg_validate_path_component(manifest->version))
    {
      return -EINVAL;
    }

  if (strlen(manifest->sha256) != PKG_HASH_HEX_LEN)
    {
      return -EINVAL;
    }

  if (!pkg_validate_hex(manifest->sha256))
    {
      return -EINVAL;
    }

  if (manifest->type != PKG_PAYLOAD_ELF &&
      manifest->type != PKG_PAYLOAD_SHARED_LIB)
    {
      return -EINVAL;
    }

  if (manifest->launch_argc > PKG_LAUNCH_ARGS_MAX)
    {
      return -EINVAL;
    }

  for (i = 0; i < manifest->launch_argc; i++)
    {
      if (pkg_validate_required(manifest->launch_args[i]) < 0)
        {
          return -EINVAL;
        }
    }

  return 0;
}

int pkg_manifest_parse_type(FAR const char *value,
                            FAR enum pkg_payload_type_e *type)
{
  if (value == NULL || type == NULL)
    {
      return -EINVAL;
    }

  if (strcmp(value, "elf") == 0)
    {
      *type = PKG_PAYLOAD_ELF;
      return 0;
    }

  if (strcmp(value, "shared-lib") == 0 ||
      strcmp(value, "shared_lib") == 0 ||
      strcmp(value, "shared") == 0)
    {
      *type = PKG_PAYLOAD_SHARED_LIB;
      return 0;
    }

  return -EINVAL;
}
