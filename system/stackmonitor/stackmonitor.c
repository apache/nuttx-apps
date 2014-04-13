/****************************************************************************
 * apps/system/stackmonitor/stackmonitor.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <syslog.h>
#include <errno.h>

#include <nuttx/arch.h>

#ifdef CONFIG_SYSTEM_STACKMONITOR

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STKMON_PREFIX "Stack Monitor: "

/* Configuration ************************************************************/

#ifndef CONFIG_SYSTEM_STACKMONITOR_STACKSIZE
#  define CONFIG_SYSTEM_STACKMONITOR_STACKSIZE 2048
#endif

#ifndef CONFIG_SYSTEM_STACKMONITOR_PRIORITY
#  define CONFIG_SYSTEM_STACKMONITOR_PRIORITY 50
#endif

#ifndef CONFIG_SYSTEM_STACKMONITOR_INTERVAL
#  define CONFIG_SYSTEM_STACKMONITOR_INTERVAL 2
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stkmon_state_s
{
  volatile bool started;
  volatile bool stop;
  pid_t pid;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stkmon_state_s g_stackmonitor;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stkmon_task
 ****************************************************************************/

static void stkmon_task(FAR struct tcb_s *tcb, FAR void *arg)
{
#if CONFIG_NAME_MAX > 0
  syslog("%5d %6d %6d %s\n",
         tcb->pid, tcb->adj_stack_size, up_check_tcbstack(tcb), tcb->name);
#else
  syslog("%5d %6d %6d\n",
         tcb->pid, tcb->adj_stack_size, up_check_tcbstack(tcb));
#endif
}

static int stackmonitor_daemon(int argc, char **argv)
{
  syslog(STKMON_PREFIX "Running: %d\n", g_stackmonitor.pid);

  /* Loop until we detect that there is a request to stop. */

  while (!g_stackmonitor.stop)
    {
      sleep(CONFIG_SYSTEM_STACKMONITOR_INTERVAL);
#if CONFIG_NAME_MAX > 0
      syslog("%-5s %-6s %-6s %s\n", "PID", "SIZE", "USED", "THREAD NAME");
#else
      syslog("%-5s %-6s %-6s\n", "PID", "SIZE", "USED");
#endif
      sched_foreach(stkmon_task, NULL);
    }

  /* Stopped */

  g_stackmonitor.stop    = false;
  g_stackmonitor.started = false;
  syslog(STKMON_PREFIX "Stopped: %d\n", g_stackmonitor.pid);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stackmonitor_start(int argc, char **argv)
{
  /* Has the monitor already started? */

  sched_lock();
  if (!g_stackmonitor.started)
    {
      int ret;

      /* No.. start it now */

      /* Then start the stack monitoring daemon */

      g_stackmonitor.started = true;
      g_stackmonitor.stop    = false;

      ret = TASK_CREATE("Stack Monitor", CONFIG_SYSTEM_STACKMONITOR_PRIORITY,
                        CONFIG_SYSTEM_STACKMONITOR_STACKSIZE,
                        (main_t)stackmonitor_daemon, (FAR char * const *)NULL);
      if (ret < 0)
        {
          int errcode = errno;
          syslog(STKMON_PREFIX
                 "ERROR: Failed to start the stack monitor: %d\n",
                 errcode);
        }
      else
        {
          g_stackmonitor.pid = ret;
          syslog(STKMON_PREFIX "Started: %d\n", g_stackmonitor.pid);
        }

      sched_unlock();
      return 0;
    }

  sched_unlock();
  syslog(STKMON_PREFIX "%s: %d\n",
         g_stackmonitor.stop ? "Stopping" : "Running", g_stackmonitor.pid);
  return 0;
}

int stackmonitor_stop(int argc, char **argv)
{
  /* Has the monitor already started? */

  if (g_stackmonitor.started)
    {
      /* Stop the stack monitor.  The next time the monitor wakes up,
       * it will see the the stop indication and will exist.
       */

      syslog(STKMON_PREFIX "Stopping: %d\n", g_stackmonitor.pid);
      g_stackmonitor.stop = true;
    }

  syslog(STKMON_PREFIX "Stopped: %d\n", g_stackmonitor.pid);
  return 0;
}

#endif /* CONFIG_SYSTEM_STACKMONITOR */
