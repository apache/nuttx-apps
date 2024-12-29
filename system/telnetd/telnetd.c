/****************************************************************************
 * apps/system/telnetd/telnetd.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "netutils/telnetd.h"
#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *argv_[] =
  {
    CONFIG_SYSTEM_TELNETD_PROGNAME,
    "-c",
    NULL,
  };

  struct telnetd_config_s config =
  {
    HTONS(CONFIG_SYSTEM_TELNETD_PORT),
#ifdef CONFIG_NET_IPv4
    AF_INET,
#else
    AF_INET6,
#endif
    CONFIG_SYSTEM_TELNETD_SESSION_PRIORITY,
    CONFIG_SYSTEM_TELNETD_SESSION_STACKSIZE,
#ifndef CONFIG_BUILD_KERNEL
    nsh_telnetmain,
#endif
#ifdef CONFIG_LIBC_EXECFUNCS
    CONFIG_SYSTEM_TELNETD_PROGNAME,
#endif
    argv_,
  };

  int daemon = 1;
  int opt;

  while ((opt = getopt(argc, argv, "46cp:")) != ERROR)
    {
      switch (opt)
        {
#ifdef CONFIG_NET_IPv4
          case '4':
            config.d_family = AF_INET;
            break;
#endif
#ifdef CONFIG_NET_IPv6
          case '6':
            config.d_family = AF_INET6;
            break;
#endif
          case 'c':
            daemon = 0;
            break;

          case 'p':
            config.d_port = atoi(optarg);
            break;

          default:
            fprintf(stderr, "Usage: %s [-4|-6] [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

  return daemon ? telnetd_daemon(&config) : nsh_telnetmain(1, argv);
}
