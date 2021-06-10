/****************************************************************************
 * apps/testing/ostest/vfork.c
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "ostest.h"

#if defined(CONFIG_ARCH_HAVE_VFORK) && defined(CONFIG_SCHED_WAITPID)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_vforkchild;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int vfork_test(void)
{
  pid_t pid;

  g_vforkchild = false;
  pid = vfork();
  if (pid == 0)
    {
      /* There is not very much that the child is permitted to do.  Perhaps
       * it can just set g_vforkchild.
       */

      g_vforkchild = true;
      exit(0);
    }
  else if (pid < 0)
    {
      printf("vfork_test: vfork() failed: %d\n", errno);
      return -1;
    }
  else
    {
      sleep(1);
      if (g_vforkchild)
        {
          printf("vfork_test: Child %d ran successfully\n", pid);
        }
      else
        {
          printf("vfork_test: ERROR Child %d did not run\n", pid);
          return -1;
        }
    }

  return 0;
}

#endif /* CONFIG_ARCH_HAVE_VFORK && CONFIG_SCHED_WAITPID */
