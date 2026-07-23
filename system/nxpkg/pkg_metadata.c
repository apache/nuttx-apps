/****************************************************************************
 * apps/system/nxpkg/pkg_metadata.c
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
#include <stdlib.h>
#include <string.h>

#include "netutils/cJSON.h"

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkg_copy_string(FAR char *dest, size_t size, FAR const char *src)
{
  int ret;

  ret = snprintf(dest, size, "%s", src);
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

static bool pkg_string_equal(FAR const char *lhs, size_t lhs_size,
                             FAR const char *rhs, size_t rhs_size)
{
  size_t lhs_len;
  size_t rhs_len;

  lhs_len = strnlen(lhs, lhs_size);
  rhs_len = strnlen(rhs, rhs_size);

  if (lhs_len >= lhs_size || rhs_len >= rhs_size)
    {
      return false;
    }

  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

static int pkg_string_cmp(FAR const char *lhs, size_t lhs_size,
                          FAR const char *rhs, size_t rhs_size)
{
  size_t lhs_len;
  size_t rhs_len;
  size_t cmp_len;

  lhs_len = strnlen(lhs, lhs_size);
  rhs_len = strnlen(rhs, rhs_size);
  cmp_len = lhs_len > rhs_len ? lhs_len : rhs_len;

  if (lhs_len >= lhs_size || rhs_len >= rhs_size)
    {
      return strncmp(lhs, rhs, cmp_len);
    }

  return strncmp(lhs, rhs, cmp_len + 1);
}

static FAR cJSON *pkg_metadata_packages_array(FAR cJSON *root)
{
  if (cJSON_IsArray(root))
    {
      return root;
    }

  return cJSON_GetObjectItemCaseSensitive(root, "packages");
}

static int pkg_metadata_parse_launch_args(
              FAR cJSON *item, FAR struct pkg_manifest_s *manifest)
{
  FAR cJSON *field;
  FAR cJSON *arg;
  size_t argc = 0;
  FAR const char *value;
  int ret;

  field = cJSON_GetObjectItemCaseSensitive(item, "launch_args");
  if (field == NULL)
    {
      manifest->launch_argc = 0;
      return 0;
    }

  if (!cJSON_IsArray(field))
    {
      return -EINVAL;
    }

  cJSON_ArrayForEach(arg, field)
    {
      if (argc >= PKG_LAUNCH_ARGS_MAX)
        {
          return -E2BIG;
        }

      value = cJSON_GetStringValue(arg);
      if (value == NULL)
        {
          return -EINVAL;
        }

      ret = pkg_copy_string(manifest->launch_args[argc],
                            sizeof(manifest->launch_args[argc]), value);
      if (ret < 0)
        {
          return ret;
        }

      argc++;
    }

  manifest->launch_argc = argc;
  return 0;
}

static int pkg_metadata_parse_manifest(FAR cJSON *item,
                                       FAR struct pkg_manifest_s *manifest)
{
  FAR cJSON *field;
  FAR const char *value;
  int ret;

  memset(manifest, 0, sizeof(*manifest));

  field = cJSON_GetObjectItemCaseSensitive(item, "name");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(manifest->name, sizeof(manifest->name),
                             value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "version");
  value = cJSON_GetStringValue(field);
  if (value == NULL || (ret = pkg_copy_string(manifest->version,
                                              sizeof(manifest->version),
                                              value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "arch");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(manifest->arch, sizeof(manifest->arch),
                             value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "compat");
  value = cJSON_GetStringValue(field);
  if (value == NULL || (ret = pkg_copy_string(manifest->compat,
                                              sizeof(manifest->compat),
                                              value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "artifact");
  value = cJSON_GetStringValue(field);
  if (value == NULL || (ret = pkg_copy_string(manifest->artifact,
                                              sizeof(manifest->artifact),
                                              value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "sha256");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(manifest->sha256, sizeof(manifest->sha256),
                             value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "type");
  value = cJSON_GetStringValue(field);
  if (pkg_manifest_parse_type(value, &manifest->type) < 0)
    {
      return -EINVAL;
    }

  /* description/category/icon are optional and purely for UI display;
   * missing fields just leave the manifest's copy empty.
   */

  field = cJSON_GetObjectItemCaseSensitive(item, "description");
  value = cJSON_GetStringValue(field);
  if (value != NULL)
    {
      pkg_copy_string(manifest->description, sizeof(manifest->description),
                      value);
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "category");
  value = cJSON_GetStringValue(field);
  if (value != NULL)
    {
      pkg_copy_string(manifest->category, sizeof(manifest->category),
                      value);
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "icon");
  value = cJSON_GetStringValue(field);
  if (value != NULL)
    {
      pkg_copy_string(manifest->icon, sizeof(manifest->icon), value);
    }

  ret = pkg_metadata_parse_launch_args(item, manifest);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_manifest_validate(manifest);
}

static int pkg_metadata_parse_versions(FAR cJSON *versions,
                                       FAR struct pkg_installed_entry_s
                                         *entry)
{
  FAR cJSON *item;
  size_t count = 0;

  if (!cJSON_IsArray(versions))
    {
      return -EINVAL;
    }

  cJSON_ArrayForEach(item, versions)
    {
      FAR const char *value;
      int ret;

      if (count >= PKG_INSTALLED_VERSIONS_MAX)
        {
          return -E2BIG;
        }

      value = cJSON_GetStringValue(item);
      if (value == NULL)
        {
          return -EINVAL;
        }

      ret = pkg_copy_string(entry->versions[count],
                            sizeof(entry->versions[count]), value);
      if (ret < 0)
        {
          return ret;
        }

      count++;
    }

  entry->version_count = count;
  return 0;
}

static int pkg_metadata_parse_installed_entry(
              FAR cJSON *item,
              FAR struct pkg_installed_entry_s *entry)
{
  FAR cJSON *field;
  FAR const char *value;
  bool current_found = false;
  bool previous_found = false;
  size_t i;
  int ret;

  memset(entry, 0, sizeof(*entry));

  field = cJSON_GetObjectItemCaseSensitive(item, "name");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(entry->name, sizeof(entry->name), value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "current");
  value = cJSON_GetStringValue(field);
  if (value == NULL || (ret = pkg_copy_string(entry->current,
                                              sizeof(entry->current),
                                              value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "previous");
  value = cJSON_GetStringValue(field);
  if (value != NULL)
    {
      ret = pkg_copy_string(entry->previous, sizeof(entry->previous),
                            value);
      if (ret < 0)
        {
          return ret;
        }
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "arch");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(entry->arch, sizeof(entry->arch), value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "compat");
  value = cJSON_GetStringValue(field);
  if (value == NULL ||
      (ret = pkg_copy_string(entry->compat, sizeof(entry->compat),
                             value)) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "type");
  value = cJSON_GetStringValue(field);
  if (pkg_manifest_parse_type(value, &entry->type) < 0)
    {
      return -EINVAL;
    }

  field = cJSON_GetObjectItemCaseSensitive(item, "versions");
  ret = pkg_metadata_parse_versions(field, entry);
  if (ret < 0)
    {
      return ret;
    }

  if (!pkg_validate_path_component(entry->name) ||
      !pkg_validate_path_component(entry->current) ||
      (entry->previous[0] != '\0' &&
       !pkg_validate_path_component(entry->previous)))
    {
      return -EINVAL;
    }

  for (i = 0; i < entry->version_count; i++)
    {
      if (!pkg_validate_path_component(entry->versions[i]))
        {
          return -EINVAL;
        }

      current_found |= strcmp(entry->versions[i], entry->current) == 0;
      previous_found |= strcmp(entry->versions[i], entry->previous) == 0;
    }

  if (!current_found ||
      (entry->previous[0] != '\0' && !previous_found))
    {
      return -EINVAL;
    }

  return 0;
}

static int pkg_metadata_version_token_cmp(FAR const char *lhs,
                                          FAR const char *rhs)
{
  long leftnum;
  long rightnum;
  FAR char *leftend;
  FAR char *rightend;
  FAR const char *cmpleft;
  FAR const char *cmpright;
  int ret;

  leftnum = strtol(lhs, &leftend, 10);
  rightnum = strtol(rhs, &rightend, 10);

  if (leftend != lhs && rightend != rhs)
    {
      if (leftnum < rightnum)
        {
          return -1;
        }

      if (leftnum > rightnum)
        {
          return 1;
        }

      /* Equal numeric prefixes don't make the tokens equal if what
       * follows the number differs - e.g. "1a" and "1b" both parse as
       * 1, but are documented to compare lexically by their non-numeric
       * component.  Falling through to "equal" here (as this used to)
       * silently treated any such pair as the same version, which is
       * exactly backwards for tokens whose whole point is to be
       * distinguishable.  Compare what's left after the numeric prefix
       * - leftend/rightend - lexically instead of falling through.
       */

      cmpleft = leftend;
      cmpright = rightend;
    }
  else
    {
      cmpleft = lhs;
      cmpright = rhs;
    }

  ret = pkg_string_cmp(cmpleft, PKG_VERSION_MAX + 1,
                       cmpright, PKG_VERSION_MAX + 1);
  if (ret < 0)
    {
      return -1;
    }

  if (ret > 0)
    {
      return 1;
    }

  return 0;
}

static int pkg_metadata_version_cmp(FAR const char *lhs, FAR const char *rhs)
{
  while (*lhs != '\0' || *rhs != '\0')
    {
      char leftbuf[16];
      char rightbuf[16];
      size_t leftlen = 0;
      size_t rightlen = 0;
      int ret;

      while (lhs[leftlen] != '\0' && lhs[leftlen] != '.')
        {
          if (leftlen + 1 >= sizeof(leftbuf))
            {
              return pkg_string_cmp(lhs, PKG_VERSION_MAX + 1,
                                    rhs, PKG_VERSION_MAX + 1);
            }

          leftbuf[leftlen] = lhs[leftlen];
          leftlen++;
        }

      while (rhs[rightlen] != '\0' && rhs[rightlen] != '.')
        {
          if (rightlen + 1 >= sizeof(rightbuf))
            {
              return pkg_string_cmp(lhs, PKG_VERSION_MAX + 1,
                                    rhs, PKG_VERSION_MAX + 1);
            }

          rightbuf[rightlen] = rhs[rightlen];
          rightlen++;
        }

      leftbuf[leftlen] = '\0';
      rightbuf[rightlen] = '\0';

      ret = pkg_metadata_version_token_cmp(leftbuf, rightbuf);
      if (ret != 0)
        {
          return ret;
        }

      lhs += leftlen;
      rhs += rightlen;

      if (*lhs == '.')
        {
          lhs++;
        }

      if (*rhs == '.')
        {
          rhs++;
        }
    }

  return 0;
}

static FAR cJSON *pkg_metadata_manifest_to_json(
                     FAR const struct pkg_manifest_s *manifest)
{
  FAR cJSON *root;
  FAR cJSON *launch_args;
  size_t i;

  root = cJSON_CreateObject();
  if (root == NULL)
    {
      return NULL;
    }

  cJSON_AddStringToObject(root, "name", manifest->name);
  cJSON_AddStringToObject(root, "version", manifest->version);
  cJSON_AddStringToObject(root, "arch", manifest->arch);
  cJSON_AddStringToObject(root, "compat", manifest->compat);
  cJSON_AddStringToObject(root, "artifact", manifest->artifact);
  cJSON_AddStringToObject(root, "sha256", manifest->sha256);
  cJSON_AddStringToObject(root, "type",
                          pkg_manifest_type_str(manifest->type));

  if (manifest->description[0] != '\0')
    {
      cJSON_AddStringToObject(root, "description", manifest->description);
    }

  if (manifest->category[0] != '\0')
    {
      cJSON_AddStringToObject(root, "category", manifest->category);
    }

  if (manifest->launch_argc > 0)
    {
      launch_args = cJSON_AddArrayToObject(root, "launch_args");
      if (launch_args == NULL)
        {
          cJSON_Delete(root);
          return NULL;
        }

      for (i = 0; i < manifest->launch_argc; i++)
        {
          FAR cJSON *arg;

          arg = cJSON_CreateString(manifest->launch_args[i]);
          if (arg == NULL)
            {
              cJSON_Delete(root);
              return NULL;
            }

          cJSON_AddItemToArray(launch_args, arg);
        }
    }

  return root;
}

static int pkg_metadata_parse_index_text(FAR const char *text,
                                         FAR struct pkg_index_s *index)
{
  FAR cJSON *root;
  FAR cJSON *packages;
  FAR cJSON *item;
  size_t count = 0;
  int ret;

  root = cJSON_Parse(text);
  pkg_info("cJSON_Parse returned %s", root != NULL ? "success" : "failure");
  if (root == NULL)
    {
      return -EINVAL;
    }

  packages = pkg_metadata_packages_array(root);
  if (!cJSON_IsArray(packages))
    {
      cJSON_Delete(root);
      return -EINVAL;
    }

  cJSON_ArrayForEach(item, packages)
    {
      if (count >= PKG_INDEX_MAX)
        {
          /* Keep what's already parsed rather than discarding the whole
           * index: a catalog that's grown past PKG_INDEX_MAX shouldn't
           * make every other package unavailable too.
           */

          pkg_error("index has more than %d packages, truncating",
                    PKG_INDEX_MAX);
          break;
        }

      ret = pkg_metadata_parse_manifest(item, &index->manifests[count]);
      if (ret < 0)
        {
          /* Skip a malformed entry instead of discarding the entire
           * index: one bad/malicious package definition shouldn't make
           * every other, otherwise-valid package unavailable too.
           */

          pkg_error("skipping malformed package entry %zu: %d", count,
                    ret);
          continue;
        }

      pkg_info("parsed manifest %s %s",
               index->manifests[count].name,
               index->manifests[count].version);
      count++;
    }

  index->count = count;
  cJSON_Delete(root);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int pkg_metadata_load_index_path(FAR const char *path,
                                 FAR struct pkg_index_s *index)
{
  FAR char *text;
  size_t textlen;
  int ret;

  if (path == NULL || index == NULL)
    {
      return -EINVAL;
    }

  memset(index, 0, sizeof(*index));

  pkg_info("loading index from %s", path);

  ret = pkg_store_read_text(path, &text);
  if (ret < 0)
    {
      return ret;
    }

  textlen = strlen(text);
  pkg_info("index read complete (%zu bytes)", textlen);

  ret = pkg_metadata_parse_index_text(text, index);
  pkg_free(text);
  return ret;
}

int pkg_metadata_load_index(FAR struct pkg_index_s *index)
{
  char path[PATH_MAX];
  int ret;

  ret = pkg_store_format_index_path(path, sizeof(path));
  if (ret < 0)
    {
      return ret;
    }

  return pkg_metadata_load_index_path(path, index);
}

int pkg_metadata_load_manifest_path(FAR const char *path,
                                    FAR struct pkg_manifest_s *manifest)
{
  FAR cJSON *root;
  FAR char *text;
  int ret;

  if (path == NULL || manifest == NULL)
    {
      return -EINVAL;
    }

  ret = pkg_store_read_text(path, &text);
  if (ret < 0)
    {
      return ret;
    }

  root = cJSON_Parse(text);
  pkg_free(text);
  if (root == NULL)
    {
      return -EINVAL;
    }

  ret = pkg_metadata_parse_manifest(root, manifest);
  cJSON_Delete(root);
  return ret;
}

FAR const struct pkg_manifest_s *
pkg_metadata_find_latest(FAR const struct pkg_index_s *index,
                         FAR const char *name)
{
  FAR const struct pkg_manifest_s *best = NULL;
  FAR const char *arch;
  FAR const char *compat;
  size_t i;

  if (index == NULL || name == NULL)
    {
      return NULL;
    }

  arch = pkg_runtime_arch();
  compat = pkg_runtime_compat();

  for (i = 0; i < index->count; i++)
    {
      FAR const struct pkg_manifest_s *candidate = &index->manifests[i];

      if (!pkg_string_equal(candidate->name, sizeof(candidate->name),
                            name, PKG_NAME_MAX + 1))
        {
          continue;
        }

      if (!pkg_string_equal(candidate->arch, sizeof(candidate->arch),
                            arch, PKG_ARCH_MAX + 1) ||
          !pkg_string_equal(candidate->compat, sizeof(candidate->compat),
                            compat, PKG_COMPAT_MAX + 1))
        {
          continue;
        }

      if (best == NULL ||
          pkg_metadata_version_cmp(candidate->version, best->version) > 0)
        {
          best = candidate;
        }
    }

  return best;
}

int pkg_metadata_load_installed(FAR struct pkg_installed_db_s *db)
{
  FAR cJSON *root;
  FAR cJSON *packages;
  FAR cJSON *item;
  FAR char *text;
  char path[PATH_MAX];
  size_t count = 0;
  int ret;

  if (db == NULL)
    {
      return -EINVAL;
    }

  memset(db, 0, sizeof(*db));

  ret = pkg_store_format_installed_path(path, sizeof(path));
  if (ret < 0)
    {
      return ret;
    }

  ret = pkg_store_read_text(path, &text);
  if (ret == -ENOENT)
    {
      return 0;
    }

  if (ret < 0)
    {
      return ret;
    }

  root = cJSON_Parse(text);
  pkg_free(text);
  if (root == NULL)
    {
      return -EINVAL;
    }

  packages = pkg_metadata_packages_array(root);
  if (!cJSON_IsArray(packages))
    {
      cJSON_Delete(root);
      return -EINVAL;
    }

  cJSON_ArrayForEach(item, packages)
    {
      if (count >= PKG_INSTALLED_MAX)
        {
          pkg_error("installed db has more than %d entries, truncating",
                    PKG_INSTALLED_MAX);
          break;
        }

      ret = pkg_metadata_parse_installed_entry(item, &db->entries[count]);
      if (ret < 0)
        {
          /* A single corrupted entry (plausible after a crash mid-write,
           * despite the atomic-write mechanism) must not make every
           * other installed package look uninstalled - that would drive
           * needless reinstalls for everything else.
           */

          pkg_error("skipping malformed installed entry %zu: %d", count,
                    ret);
          continue;
        }

      count++;
    }

  db->count = count;
  cJSON_Delete(root);
  return 0;
}

int pkg_metadata_save_installed(FAR const struct pkg_installed_db_s *db)
{
  FAR cJSON *root;
  FAR cJSON *packages;
  FAR char *text;
  char path[PATH_MAX];
  size_t i;
  int ret;

  if (db == NULL)
    {
      return -EINVAL;
    }

  root = cJSON_CreateObject();
  if (root == NULL)
    {
      return -ENOMEM;
    }

  packages = cJSON_AddArrayToObject(root, "packages");
  if (packages == NULL)
    {
      cJSON_Delete(root);
      return -ENOMEM;
    }

  for (i = 0; i < db->count; i++)
    {
      FAR const struct pkg_installed_entry_s *entry = &db->entries[i];
      FAR cJSON *item = cJSON_CreateObject();
      FAR cJSON *versions = cJSON_AddArrayToObject(item, "versions");
      size_t j;

      if (item == NULL || versions == NULL)
        {
          cJSON_Delete(root);
          return -ENOMEM;
        }

      cJSON_AddStringToObject(item, "name", entry->name);
      cJSON_AddStringToObject(item, "current", entry->current);
      cJSON_AddStringToObject(item, "previous", entry->previous);
      cJSON_AddStringToObject(item, "arch", entry->arch);
      cJSON_AddStringToObject(item, "compat", entry->compat);
      cJSON_AddStringToObject(item, "type",
                              pkg_manifest_type_str(entry->type));

      for (j = 0; j < entry->version_count; j++)
        {
          cJSON_AddItemToArray(versions,
                               cJSON_CreateString(entry->versions[j]));
        }

      cJSON_AddItemToArray(packages, item);
    }

  text = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (text == NULL)
    {
      return -ENOMEM;
    }

  ret = pkg_store_format_installed_path(path, sizeof(path));
  if (ret < 0)
    {
      cJSON_free(text);
      return ret;
    }

  ret = pkg_store_write_text_atomic(path, text);
  cJSON_free(text);
  return ret;
}

FAR struct pkg_installed_entry_s *
pkg_metadata_find_installed(FAR struct pkg_installed_db_s *db,
                            FAR const char *name)
{
  size_t i;

  if (db == NULL || name == NULL)
    {
      return NULL;
    }

  for (i = 0; i < db->count; i++)
    {
      if (pkg_string_equal(db->entries[i].name, sizeof(db->entries[i].name),
                           name, PKG_NAME_MAX + 1))
        {
          return &db->entries[i];
        }
    }

  return NULL;
}

int pkg_metadata_write_manifest(FAR const char *path,
                                FAR const struct pkg_manifest_s *manifest)
{
  FAR cJSON *root;
  FAR char *text;
  int ret;

  root = pkg_metadata_manifest_to_json(manifest);
  if (root == NULL)
    {
      return -ENOMEM;
    }

  text = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (text == NULL)
    {
      return -ENOMEM;
    }

  ret = pkg_store_write_text_atomic(path, text);
  cJSON_free(text);
  return ret;
}

int pkg_metadata_print_installed(FAR FILE *stream,
                                 FAR const struct pkg_installed_db_s *db)
{
  size_t i;

  if (stream == NULL || db == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < db->count; i++)
    {
      FAR const struct pkg_installed_entry_s *entry = &db->entries[i];
      size_t j;

      fprintf(
          stream,
          "%s current=%s previous=%s type=%s arch=%s compat=%s versions=",
          entry->name,
          entry->current[0] != '\0' ? entry->current : "-",
          entry->previous[0] != '\0' ? entry->previous : "-",
          pkg_manifest_type_str(entry->type),
          entry->arch,
          entry->compat);

      for (j = 0; j < entry->version_count; j++)
        {
          fprintf(stream, "%s%s", j == 0 ? "" : ",", entry->versions[j]);
        }

      fputc('\n', stream);
    }

  return 0;
}
