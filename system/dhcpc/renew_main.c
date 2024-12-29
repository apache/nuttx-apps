/****************************************************************************
 * apps/system/dhcpc/renew_main.c
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

#include <stdlib.h>
#include <stdio.h>

#include <net/if.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * dhcpc_showusage
 ****************************************************************************/

static void dhcpc_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage: %s <device-name>\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * renew_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  /* One and only one argument is expected:  The network device name. */

  if (argc != 2)
    {
      fprintf(stderr, "ERROR: Invalid number of arguments\n");
      dhcpc_showusage(argv[0], EXIT_FAILURE);
    }

  ret = netlib_obtain_ipv4addr(argv[1]);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_obtain_ipv4addr() failed\n");
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
