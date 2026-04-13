/****************************************************************************
 * apps/system/dumpstack/dumpstack.c
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

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <execinfo.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DUMP_DEPTH 16

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR void *backtrace_buf[DUMP_DEPTH];
  char fmt[BACKTRACE_BUFFER_SIZE(DUMP_DEPTH)];
  int spid = -1;
  int epid = -1;
  int tmp = -1;
  int depth;

  if (argc == 2)
    {
      spid = epid = atoi(argv[1]);
    }
  else if (argc >= 3)
    {
      spid = atoi(argv[1]);
      epid = atoi(argv[2]);
    }

  if (spid < 0 || epid < 0)
    {
      spid = epid = gettid();
    }

  if (spid > epid)
    {
      tmp = spid;
      spid = epid;
      epid = tmp;
    }

  do
    {
      depth = sched_backtrace(spid, backtrace_buf, DUMP_DEPTH, 0);
      if (depth > 0)
        {
          backtrace_format(fmt, sizeof(fmt), backtrace_buf, depth);
          printf("backtrace|%d: %s\n", spid, fmt);
        }
    }
  while (spid++ != epid);

  return 0;
}
