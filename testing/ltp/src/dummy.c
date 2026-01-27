/****************************************************************************
 * apps/testing/ltp/src/dummy.c
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
#include <unistd.h>
#include <string.h>
#include <mntent.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* the following implementation build when CONFIG_SYSTEM_POPEN option
 * is disabled
 */

#ifndef CONFIG_SYSTEM_POPEN

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FILE *popen(const char *command, const char *mode)
{
  return NULL;
}

int pclose(FILE *stream)
{
  return OK;
}
#endif

/* the following empty implementation using to compile the LTP Kernel case
 * on nuttx
 */

int chroot(const char *path)
{
  return -1;
}

int setpgid(pid_t pid, pid_t pgid)
{
  return -1;
}

FILE *setmntent(const char *filep, const char *type)
{
  return NULL;
}

struct mntent *getmntent(FILE *filep)
{
  return NULL;
}

struct mntent *getmntent_r(FILE *, struct mntent *, char *, int)
{
  return NULL;
}

int endmntent(FILE *filep)
{
  return -1;
}

char *hasmntopt(const struct mntent *mnt, const char *opt)
{
  return NULL;
}

/* the following are dummy implementation using to compile fsgid related
 * cased on nuttx
 */

int setfsgid(uid_t fsgid)
{
  return -1;
}

int setfsuid(uid_t fsuid)
{
  return -1;
}

/* the following syscalls are needed by:
 * lib/tst_safe_macros.c
 * lib/safe_macros.c
 * lib/parse_opts.c
 */

int mincore(void *addr, size_t length, unsigned char *vec)
{
  return -1;
}

pid_t setsid(void)
{
  return -1;
}

int brk(void *addr)
{
  return -1;
}

int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
  return -1;
}

int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
  return -1;
}
