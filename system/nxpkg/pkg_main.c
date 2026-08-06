/****************************************************************************
 * apps/system/nxpkg/pkg_main.c
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netutils/cJSON.h>

#include "pkg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PKG_USAGE \
  "Usage: %s <install|update|remove|rollback|list|available|sync|help> " \
  "[args]\n"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *cmd;
  cJSON_Hooks hooks;

  hooks.malloc_fn = malloc;
  hooks.free_fn = free;
  cJSON_InitHooks(&hooks);

  if (argc < 2)
    {
      fprintf(stderr, PKG_USAGE, argv[0]);
      return EXIT_FAILURE;
    }

  cmd = argv[1];

  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 ||
      strcmp(cmd, "-h") == 0)
    {
      fprintf(stdout, PKG_USAGE, argv[0]);
      return EXIT_SUCCESS;
    }

  if (strcmp(cmd, "install") == 0)
    {
      if (argc != 3)
        {
          pkg_error("install expects exactly one package name");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      /* pkg_install() returns a real negative errno on the meaningful
       * pipeline failures (nxstore uses that directly), not just
       * EXIT_SUCCESS/EXIT_FAILURE - normalize to a plain 0/1 shell exit
       * status here.
       */

      return pkg_install(argv[2]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

  /* "update" is a package name resolving to whatever version is latest
   * in the local index - pkg_install() already handles the "already
   * installed at a different version" transition transparently via
   * pkg_install_update_installed(), so no separate code path is needed.
   */

  if (strcmp(cmd, "update") == 0)
    {
      if (argc != 3)
        {
          pkg_error("update expects exactly one package name");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_install(argv[2]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

  if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "uninstall") == 0)
    {
      if (argc != 3)
        {
          pkg_error("remove expects exactly one package name");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_uninstall(argv[2]);
    }

  if (strcmp(cmd, "rollback") == 0)
    {
      if (argc != 3)
        {
          pkg_error("rollback expects exactly one package name");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_rollback(argv[2]);
    }

  if (strcmp(cmd, "list") == 0)
    {
      if (argc != 2)
        {
          pkg_error("list does not take additional arguments");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_list(stdout);
    }

  if (strcmp(cmd, "available") == 0)
    {
      if (argc != 2)
        {
          pkg_error("available does not take additional arguments");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_available(stdout);
    }

  if (strcmp(cmd, "sync") == 0)
    {
      if (argc != 3)
        {
          pkg_error("sync expects exactly one index source");
          fprintf(stderr, PKG_USAGE, argv[0]);
          return EXIT_FAILURE;
        }

      return pkg_sync(argv[2]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

  fprintf(stderr, "ERROR: Unknown subcommand '%s'\n", cmd);
  fprintf(stderr, PKG_USAGE, argv[0]);
  return EXIT_FAILURE;
}
