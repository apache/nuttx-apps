/****************************************************************************
 * apps/testing/sched/spawnattr/spawnattr_main.c
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

/* posix_spawn() with a spawn attribute that the kernel refuses after it
 * made the new task: POSIX_SPAWN_SETSCHEDPARAM with priority 0 gives
 * EINVAL.  posix_spawn() must return the error, and the memory of the
 * caller must stay intact: the heap (a pattern and mallinfo()) after many
 * such errors, then a normal posix_spawn().  The child is this program
 * again.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <malloc.h>
#include <sched.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NAME        "spawnattr"
#define ROUNDS      100
#define NBLOCKS     16
#define CHILD_EXIT  7

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Allocate blocks of 16 bytes to 64 KiB, fill them, check them, free
 * them.  Return false if a block is missing or changed.
 */

/* posix_spawn() of the program with priority 0: return its result */

static int spawn_bad(FAR char *path, FAR char *cargv[])
{
  struct sched_param param;
  posix_spawnattr_t attr;
  pid_t pid = 0;
  int ret;

  posix_spawnattr_init(&attr);
  memset(&param, 0, sizeof(param));
  posix_spawnattr_setschedparam(&attr, &param);
  posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
  ret = posix_spawn(&pid, path, NULL, &attr, cargv, NULL);
  posix_spawnattr_destroy(&attr);
  if (ret == 0)
    {
      waitpid(pid, NULL, 0);
    }

  return ret;
}

static bool heap_round(int r)
{
  FAR unsigned char *p[NBLOCKS];
  bool ok = true;
  size_t n;
  size_t j;
  int i;

  for (i = 0; i < NBLOCKS; i++)
    {
      n = (size_t)16 << (i % 13);
      p[i] = malloc(n);
      if (p[i] == NULL)
        {
          ok = false;
          continue;
        }

      memset(p[i], (r + i) & 0xff, n);
    }

  for (i = 0; i < NBLOCKS; i++)
    {
      n = (size_t)16 << (i % 13);
      for (j = 0; p[i] != NULL && j < n; j++)
        {
          if (p[i][j] != ((r + i) & 0xff))
            {
              ok = false;
              break;
            }
        }

      free(p[i]);
    }

  return ok;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *cargv[3];
  struct mallinfo before;
  struct mallinfo after;
  pid_t pid = 0;
  int status = -1;
  int fails = 0;
  int bad = 0;
  bool heap = true;
  bool ok;
  int ret;
  int r;

  if (argc > 1 && strcmp(argv[1], "child") == 0)
    {
      return CHILD_EXIT;
    }

  cargv[0] = argv[0];
  cargv[1] = "child";
  cargv[2] = NULL;

  /* The first posix_spawn() may set up state that stays: measure after it */

  bad += spawn_bad(argv[0], cargv) != EINVAL;
  heap_round(0);
  before = mallinfo();

  for (r = 1; r < ROUNDS; r++)
    {
      bad += spawn_bad(argv[0], cargv) != EINVAL;
      heap &= heap_round(r);
    }

  after = mallinfo();

  ok = bad == 0;
  printf(NAME ": prio0     %d posix_spawn() with priority 0: %d not EINVAL "
         "%s\n", ROUNDS, bad, ok ? "PASS" : "FAIL");
  fails += !ok;

  printf(NAME ": heap      pattern after each error %s %s\n",
         heap ? "intact" : "changed", heap ? "PASS" : "FAIL");
  fails += !heap;

  ok = before.uordblks == after.uordblks &&
       before.fordblks == after.fordblks;
  printf(NAME ": mallinfo  uordblks %d -> %d, fordblks %d -> %d %s\n",
         before.uordblks, after.uordblks, before.fordblks, after.fordblks,
         ok ? "PASS" : "FAIL");
  fails += !ok;

  ret = posix_spawn(&pid, argv[0], NULL, NULL, cargv, NULL);
  if (ret == 0 && waitpid(pid, &status, 0) != pid)
    {
      status = -1;
    }

  ok = ret == 0 && WIFEXITED(status) && WEXITSTATUS(status) == CHILD_EXIT;
  printf(NAME ": good      posix_spawn() %d, exit %d %s\n", ret,
         WIFEXITED(status) ? WEXITSTATUS(status) : -1, ok ? "PASS" : "FAIL");
  fails += !ok;

  printf(NAME ": done, %d failed\n", fails);
  return fails != 0;
}
