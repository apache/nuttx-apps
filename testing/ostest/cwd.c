/****************************************************************************
 * apps/testing/ostest/cwd.c
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

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CWD_DIR "/ostest_cwd"
#define CWD_NEW "/ostest_cwd2"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int cwd_fail(FAR const char *msg)
{
  printf("cwd_test: ERROR %s, errno=%d\n", msg, errno);
  return 1;
}

static bool cwd_is(FAR const char *expect)
{
  char buf[PATH_MAX];

  return getcwd(buf, sizeof(buf)) != NULL && strcmp(buf, expect) == 0;
}

#ifdef CONFIG_FS_CWD_TRACKING
static bool cwd_removed(void)
{
  char buf[PATH_MAX];

  errno = 0;
  return getcwd(buf, sizeof(buf)) == NULL && errno == ENOENT;
}
#endif

#ifndef CONFIG_DISABLE_PTHREAD
static FAR void *cwd_thread(FAR void *arg)
{
  UNUSED(arg);

  chdir("/");
  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int cwd_test(void)
{
  char saved[PATH_MAX];
  FAR char *path;
#ifndef CONFIG_DISABLE_PTHREAD
  pthread_t thread;
#endif
  int errors = 0;

  if (getcwd(saved, sizeof(saved)) == NULL)
    {
      strlcpy(saved, "/", sizeof(saved));
    }

  rmdir(CWD_DIR);
  rmdir(CWD_NEW);
  if (mkdir(CWD_DIR, 0777) < 0)
    {
      return cwd_fail("mkdir() failed");
    }

  if (chdir(CWD_DIR) < 0 || !cwd_is(CWD_DIR))
    {
      errors += cwd_fail("getcwd() does not give the chdir() directory");
    }

  /* getcwd(NULL, 0) allocates the buffer in the heap of the caller */

  path = getcwd(NULL, 0);
  if (path == NULL || strcmp(path, CWD_DIR) != 0)
    {
      errors += cwd_fail("getcwd(NULL, 0) does not give the directory");
    }

  free(path);

#ifndef CONFIG_DISABLE_ENVIRON
  /* PWD is information for the shell, not the current directory */

  setenv("PWD", "/", 1);
  if (!cwd_is(CWD_DIR))
    {
      errors += cwd_fail("setenv(\"PWD\") changed the current directory");
    }

  unsetenv("PWD");
  if (!cwd_is(CWD_DIR))
    {
      errors += cwd_fail("unsetenv(\"PWD\") changed the current directory");
    }
#endif

#ifndef CONFIG_DISABLE_PTHREAD
  /* The threads of a task group share the current directory */

  if (pthread_create(&thread, NULL, cwd_thread, NULL) != 0 ||
      pthread_join(thread, NULL) != 0 || !cwd_is("/"))
    {
      errors += cwd_fail("chdir() of a thread did not change the group");
    }

  chdir(CWD_DIR);
#endif

#ifdef CONFIG_FS_CWD_TRACKING
  /* rename() of the current directory: it keeps the directory */

  if (rename(CWD_DIR, CWD_NEW) < 0 || !cwd_is(CWD_NEW))
    {
      errors += cwd_fail("getcwd() does not give the new name");
    }

  /* rmdir() of the current directory: getcwd() fails with ENOENT, also
   * after a new directory of the same name appears.
   */

  if (rmdir(CWD_NEW) < 0 || !cwd_removed())
    {
      errors += cwd_fail("getcwd() does not fail after rmdir()");
    }
  else if (mkdir(CWD_NEW, 0777) < 0 || !cwd_removed())
    {
      errors += cwd_fail("a new directory became the current directory");
    }
#endif

  chdir(saved);
#ifndef CONFIG_DISABLE_ENVIRON
  setenv("PWD", saved, 1);
#endif
  rmdir(CWD_DIR);
  rmdir(CWD_NEW);

  if (errors == 0)
    {
      printf("cwd_test: PASS\n");
    }

  return errors;
}
