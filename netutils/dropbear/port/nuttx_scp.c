/****************************************************************************
 * apps/netutils/dropbear/port/nuttx_scp.c
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

#include "nuttx_scp.h"

#include <errno.h>
#include <sys/types.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* scp only runs in server mode (-t/-f) on NuttX; execvp would only be
 * reached in client mode to spawn a local ssh, which is not supported.
 */

int dropbear_scp_execvp(FAR const char *file, FAR char * const argv[])
{
  (void)file;
  (void)argv;

  set_errno(ENOSYS);
  return ERROR;
}
