/****************************************************************************
 * apps/system/nxpkg/pkg_txn.c
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

#include "pkg.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

const char *pkg_txn_state_str(enum pkg_txn_state_e state)
{
  switch (state)
    {
      case PKG_TXN_IDLE:
        return "IDLE";

      case PKG_TXN_FETCHING:
        return "FETCHING";

      case PKG_TXN_VERIFIED:
        return "VERIFIED";

      case PKG_TXN_STAGED:
        return "STAGED";

      case PKG_TXN_COMPAT_OK:
        return "COMPAT_OK";

      case PKG_TXN_ACTIVATED:
        return "ACTIVATED";

      case PKG_TXN_CLEANUP:
        return "CLEANUP";

      case PKG_TXN_FAILED:
        return "FAILED";

      case PKG_TXN_RESTORE:
        return "RESTORE";

      default:
        return "UNKNOWN";
    }
}

int pkg_txn_write_state(FAR const char *name, enum pkg_txn_state_e state)
{
  char path[PATH_MAX];
  char text[32];
  int ret;

  ret = pkg_store_format_txn_path(path, sizeof(path), name);
  if (ret < 0)
    {
      return ret;
    }

  ret = snprintf(text, sizeof(text), "%s\n", pkg_txn_state_str(state));
  if (ret < 0)
    {
      return ret;
    }

  if ((size_t)ret >= sizeof(text))
    {
      return -ENAMETOOLONG;
    }

  return pkg_store_write_text_atomic(path, text);
}

int pkg_txn_clear_state(FAR const char *name)
{
  char path[PATH_MAX];
  int ret;

  ret = pkg_store_format_txn_path(path, sizeof(path), name);
  if (ret < 0)
    {
      return ret;
    }

  return pkg_store_remove_file(path);
}
