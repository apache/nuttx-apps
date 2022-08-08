/****************************************************************************
 * apps/system/telnetd/telnetd.c
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
#include <string.h>

#include "netutils/telnetd.h"
#include "netutils/netlib.h"
#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *argv1[3];
  char arg0[sizeof("0x1234567812345678")];
  struct telnetd_s daemon;

  /* Initialize the daemon structure */

  daemon.port      = HTONS(23);
  daemon.family    = AF_INET;
  daemon.entry     = nsh_telnetmain;

  /* NOTE: Settings for telnet session task */

  daemon.priority  = CONFIG_SYSTEM_TELNETD_SESSION_PRIORITY;
  daemon.stacksize = CONFIG_SYSTEM_TELNETD_SESSION_STACKSIZE;

  snprintf(arg0, sizeof(arg0), "0x%" PRIxPTR, (uintptr_t)&daemon);
  argv1[0] = "telnetd";
  argv1[1] = arg0;
  argv1[2] = NULL;

  telnetd_daemon(2, argv1);
  return 0;
}
