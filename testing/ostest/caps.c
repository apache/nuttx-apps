/****************************************************************************
 * apps/testing/ostest/caps.c
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
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAPS_PRIO 100

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int caps_fail(FAR const char *msg)
{
  printf("caps_test: ERROR %s errno=%d\n", msg, errno);
  ASSERT(false);
  return EXIT_FAILURE;
}

static int caps_wait(pid_t pid)
{
  int status;

  if (pid < 0 || waitpid(pid, &status, 0) != pid)
    {
      return ERROR;
    }

  return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS ?
         OK : ERROR;
}

static int caps_grandchild(int argc, FAR char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  return prctl(PR_CAPS_GET) == (PR_CAP_ALL & ~PR_CAP_ADMIN) ?
         EXIT_SUCCESS : EXIT_FAILURE;
}

static int caps_child(int argc, FAR char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  if (prctl(PR_CAPS_GET) != PR_CAP_ALL)
    {
      return caps_fail("child does not start with all caps");
    }

  prctl(PR_CAPS_DROP, PR_CAP_ADMIN);
  if (caps_wait(task_create("caps_grandchild", CAPS_PRIO, STACKSIZE,
                            caps_grandchild, NULL)) < 0)
    {
      return caps_fail("grandchild did not inherit the set");
    }

  prctl(PR_CAPS_DROP, PR_CAP_SPAWN);
  if (task_create("caps_grandchild", CAPS_PRIO, STACKSIZE,
                  caps_grandchild, NULL) >= 0 || errno != EPERM)
    {
      return caps_fail("task_create without PR_CAP_SPAWN");
    }

  prctl(PR_CAPS_DROP, PR_CAP_RAWIO);
#ifndef CONFIG_DISABLE_MOUNTPOINT
  if (mount(NULL, "/caps", "tmpfs", 0, NULL) == 0 ||
      (errno != EPERM && errno != ENOSYS))
    {
      return caps_fail("mount without PR_CAP_RAWIO");
    }
#endif

  if (prctl(PR_CAPS_GET) != 0)
    {
      return caps_fail("caps left after dropping all");
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int caps_test(void)
{
  printf("caps_test: Starting test\n");

  if (caps_wait(task_create("caps_child", CAPS_PRIO, STACKSIZE,
                            caps_child, NULL)) < 0 ||
      prctl(PR_CAPS_GET) != PR_CAP_ALL)
    {
      printf("caps_test: ERROR\n");
      ASSERT(false);
      return ERROR;
    }

  printf("caps_test: PASSED\n");
  return OK;
}
