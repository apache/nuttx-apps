/****************************************************************************
 * apps/system/nxpkg/pkg_compat.c
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
#include <string.h>

#include "pkg.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

const char *pkg_runtime_arch(void)
{
  return CONFIG_ARCH;
}

const char *pkg_runtime_compat(void)
{
  return CONFIG_ARCH_BOARD;
}

int pkg_compat_check(FAR const struct pkg_manifest_s *manifest)
{
  if (manifest == NULL)
    {
      return -EINVAL;
    }

  if (strcmp(manifest->arch, pkg_runtime_arch()) != 0)
    {
      return -ENOEXEC;
    }

  if (strcmp(manifest->compat, pkg_runtime_compat()) != 0)
    {
      return -EXDEV;
    }

  return 0;
}
