/****************************************************************************
 * apps/system/brctl/brctl.c
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

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void brctl_usage(FAR const char *progname)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s addbr <bridge>\n", progname);
  fprintf(stderr, "  %s delbr <bridge>\n", progname);
  fprintf(stderr, "  %s addif <bridge> <device>\n", progname);
  fprintf(stderr, "  %s delif <bridge> <device>\n", progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *cmd;
  int ret;

  if (argc < 3)
    {
      brctl_usage(argv[0]);
      return EXIT_FAILURE;
    }

  cmd = argv[1];

  if (strcmp(cmd, "addbr") == 0 && argc == 3)
    {
      ret = netlib_add_bridge(argv[2]);
    }
  else if (strcmp(cmd, "delbr") == 0 && argc == 3)
    {
      ret = netlib_del_bridge(argv[2]);
    }
  else if (strcmp(cmd, "addif") == 0 && argc == 4)
    {
      ret = netlib_bridge_addif(argv[2], argv[3]);
    }
  else if (strcmp(cmd, "delif") == 0 && argc == 4)
    {
      ret = netlib_bridge_delif(argv[2], argv[3]);
    }
  else
    {
      brctl_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (ret < 0)
    {
      fprintf(stderr, "ERROR: brctl %s failed: %s\n", cmd,
              strerror(-ret));
    }

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
