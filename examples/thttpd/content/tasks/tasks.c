/****************************************************************************
 * apps/examples/thttpd/content/tasks/tasks.c
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
#include <sched.h>

#include <nuttx/sched.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_statenames[] =
{
  "INVALID ",
  "PENDING ",
  "READY   ",
  "RUNNING ",
  "INACTIVE",
  "WAITSEM ",
#ifndef CONFIG_DISABLE_MQUEUE
  "WAITSIG ",
#endif
#ifndef CONFIG_DISABLE_MQUEUE
  "MQNEMPTY",
  "MQNFULL "
#endif
};

static const char *g_ttypenames[4] =
{
  "TASK   ",
  "PTHREAD",
  "KTHREAD",
  "--?--  "
};

static FAR const char *g_policynames[4] =
{
  "FIFO",
  "RR  ",
  "SPOR",
  "OTHR"
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* NOTEs:
 *
 * 1. One limitation in the use of NXFLAT is that functions that are
 *    referenced as a pointer-to-a-function must have global scope.
 *    Otherwise ARM GCC will generate some bad logic.
 * 2. In general, when called back, there is no guarantee to that PIC
 *    registers will be valid and, unless you take special precautions, it
 *    could be dangerous to reference global variables in the callback
 *    function.
 */

void show_task(FAR struct tcb_s *tcb, FAR void *arg)
{
  FAR const char *policy;

  /* Show task/thread status */

  policy = g_policynames[(tcb->flags & TCB_FLAG_POLICY_MASK) >>
                         TCB_FLAG_POLICY_SHIFT];
#if CONFIG_TASK_NAME_SIZE > 0
  printf("%5d %3d %4s %7s%c%c %8s %s\n",
#else
  printf("%5d %3d %4s %7s%c%c %8s\n",
#endif
         tcb->pid, tcb->sched_priority, policy,
         g_ttypenames[(tcb->flags & TCB_FLAG_TTYPE_MASK) >>
                      TCB_FLAG_TTYPE_SHIFT],
         tcb->flags & TCB_FLAG_NONCANCELABLE ? 'N' : ' ',
         tcb->flags & TCB_FLAG_CANCEL_PENDING ? 'P' : ' ',
         g_statenames[tcb->task_state]
#if CONFIG_TASK_NAME_SIZE > 0
         , tcb->name
#endif
         );
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_THTTPD_BINFS
int tasks_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  puts(
    "Content-type: text/html\r\n"
    "Status: 200/html\r\n"
    "\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>NuttX Tasks</title>\r\n"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\r\n"
    "</head>\r\n"
    "<body bgcolor=\"#fffeec\" text=\"black\">\r\n"
    "<div class=\"menu\">\r\n"
    "<div class=\"menubox\"><a href=\"/index.html\">Front page</a></div>\r\n"
    "<div class=\"menubox\"><a href=\"hello\">Say Hello</a></div>\r\n"
    "<div class=\"menubox\"><a href=\"tasks\">Tasks</a></div>\r\n"
    "<br>\r\n"
    "</div>\r\n"
    "<div class=\"contentblock\">\r\n"
    "<pre>\r\n"
    "PID   PRI SCHD TYPE   NP STATE    NAME\r\n");

  nxsched_foreach(show_task, NULL);

  puts(
    "</pre>\r\n"
    "</body>\r\n"
    "</html>\r\n");

  return 0;
}
