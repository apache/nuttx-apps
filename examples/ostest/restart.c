/****************************************************************************
 * examples/ostest/restart.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PRIORITY    100
#define NARGS         3

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char * const g_argv[NARGS+1] =
{
  "This is argument 1",
  "Argument 2 here",
  "Lastly, the 3rd argument",
  NULL
};

#ifndef CONFIG_DISABLE_ENVIRON
static const char g_varname[] = "VarName";
static const char g_varvalue[] = "VarValue";
#endif

static bool g_restarted;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int restart_main(int argc, char *argv[])
{
#ifndef CONFIG_DISABLE_ENVIRON
  char *actual;
#endif
  int i;

  printf("restart_main: Started with argc=%d\n", argc);

  /* Verify passed arguments */

  if (argc != NARGS + 1)
    {
      printf("restart_main: ERROR: Expected argc=%d got argc=%d\n",
             NARGS+1, argc);
    }

  for (i = 0; i <= NARGS; i++)
    {
      printf("restart_main: argv[%d]=\"%s\"\n", i, argv[i]);
      if (i > 0 && strcmp(argv[i], g_argv[i-1]) != 0)
        {
          printf("restart_main: ERROR: Expected argv[%d]=\"%s\" got \"%s\"\n",
                 i, argv[i], g_argv[i-1]);
        }
    }

#ifndef CONFIG_DISABLE_ENVIRON
  actual = getenv(g_varname);
  if (actual)
    {
      if (strcmp(actual, g_varvalue) == 0)
        {
          printf("restart_main: Variable=%s has value=%s\n", g_varname,
                 g_varvalue);
        }
      else
        {
          printf("restart_main: ERROR Variable=%s has the wrong value\n", g_varname);
          printf("restart_main:       found=%s expected=%s\n", actual, g_varvalue);
        }
    }
  else
    {
      printf("restart_main: ERROR: Variable=%s has no value\n", g_varname);
    }
#endif

  /* Were we restarted? */

  if (!g_restarted)
    {
      /* No.. this is the first time we have been here */

      g_restarted = true;

      /* Now just wait to be restarted */

      for (;;)
        {
          sleep(2);
          printf("restart_main: I am still here\n");
        }
    }

  return 0; /* Won't get here unless we were restarted */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void restart_test(void)
{
  int ret;

  /* Start the children and wait for first one to complete */

  printf("\nTest task_restart()\n");
  g_restarted = false;

  /* Set up an environment variables */

#ifndef CONFIG_DISABLE_ENVIRON
  printf("restart_main: setenv(%s, %s, TRUE)\n", g_varname, g_varvalue);
  setenv(g_varname, g_varvalue, TRUE);  /* Variable1=GoodValue1 */
#endif

  /* Start the task */

  ret = task_create("ostest", PRIORITY, STACKSIZE, restart_main, g_argv);
  if (ret < 0)
    {
      printf("restart_main: ERROR Failed to start restart_main\n");
    }
  else
    {
      pid_t pid = ret;

      printf("restart_main: Started restart_main at PID=%d\n", pid);

      /* Wait a bit and restart the task */

      sleep(5);
      ret = task_restart(pid);
      if (ret < 0)
        {
          printf("restart_main:  ERROR: task_restart failed\n");
        }

      sleep(1);
    }

  printf("restart_main: Exiting\n");
}
