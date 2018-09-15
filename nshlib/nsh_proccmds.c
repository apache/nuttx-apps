/****************************************************************************
 * apps/nshlib/nsh_proccmds.c
 *
 *   Copyright (C) 2007-2009, 2011-2012, 2014-2015 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_NSH_PROC_MOUNTPOINT
#  define CONFIG_NSH_PROC_MOUNTPOINT "/proc"
#endif

/* See include/nuttx/sched.h: */

#undef HAVE_GROUPID

#if defined(CONFIG_SCHED_HAVE_PARENT) && defined(CONFIG_SCHED_CHILD_STATUS)
#  define HAVE_GROUPID  1
#endif

#ifdef CONFIG_DISABLE_PTHREAD
#  undef HAVE_GROUPID
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The returned value should be zero for success or TRUE or non zero for
 * failure or FALSE.
 */

typedef int (*exec_t)(void);

/* This structure represents the parsed task characteristics */

struct nsh_taskstatus_s
{
  FAR const char *td_type;       /* Thread type */
#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
  FAR const char *td_groupid;    /* Group ID */
#else
  FAR const char *td_ppid;       /* Parent thread ID */
#endif
#endif
#ifdef CONFIG_SMP
  FAR const char *td_cpu;        /* CPU */
#endif
  FAR const char *td_state;      /* Thread state */
  FAR const char *td_event;      /* Thread wait event */
  FAR const char *td_flags;      /* Thread flags */
  FAR const char *td_priority;   /* Thread priority */
  FAR const char *td_policy;     /* Thread scheduler */
#ifndef CONFIG_DISABLE_SIGNALS
  FAR const char *td_sigmask;    /* Signal mask */
#endif
};

/* Status strings */

#ifndef CONFIG_NSH_DISABLE_PS
#if 0 /* Not used */
static const char g_name[]      = "Name:";
#endif

static const char g_type[]      = "Type:";

#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
static const char g_groupid[]   = "Group:";
#else
static const char g_ppid[]      = "PPID:";
#endif
#endif /* CONFIG_SCHED_HAVE_PARENT */

#ifdef CONFIG_SMP
static const char g_cpu[]       = "CPU:";
#endif

static const char g_state[]     = "State:";
static const char g_flags[]     = "Flags:";
static const char g_priority[]  = "Priority:";
static const char g_scheduler[] = "Scheduler:";

#ifndef CONFIG_DISABLE_SIGNALS
static const char g_sigmask[]   = "SigMask:";
#endif

#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
static const char g_stacksize[] = "StackSize:";
#ifdef CONFIG_STACK_COLORATION
static const char g_stackused[] = "StackUsed:";
#endif
#endif /* !CONFIG_NSH_DISABLE_PSSTACKUSAGE */
#endif /* !CONFIG_NSH_DISABLE_PS */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_parse_statusline
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
static void nsh_parse_statusline(FAR char *line,
                                 FAR struct nsh_taskstatus_s *status)
{
  /* Parse the task status.
   *
   *   Format:
   *
   *            111111111122222222223
   *   123456789012345678901234567890
   *   Name:       xxxx...            Task/thread name (See CONFIG_TASK_NAME_SIZE)
   *   Type:       xxxxxxx            {Task, pthread, Kthread, Invalid}
   *   Type:       xxxxxxx            {Task, pthread, Kthread, Invalid}
   *   PPID:       xxxxx              Parent thread ID
   *   State:      xxxxxxxx,xxxxxxxxx {Invalid, Waiting, Ready, Running, Inactive},
   *                                  {Unlock, Semaphore, Signal, MQ empty, MQ full}
   *   Flags:      xxx                N,P,X
   *   Priority:   nnn                Decimal, 0-255
   *   Scheduler:  xxxxxxxxxxxxxx     {SCHED_FIFO, SCHED_RR, SCHED_SPORADIC, SCHED_OTHER}
   *   Sigmask:    nnnnnnnn           Hexadecimal, 32-bit
   */

#if 0 /* Not used */
 /* Task name */

 if (strncmp(line, g_name, strlen(g_name)) == 0)
   {
     /* Not used */
   }
 else
#endif
  /* Task/thread type */

  if (strncmp(line, g_type, strlen(g_type)) == 0)
    {
      /* Save the thread type */

      status->td_type = nsh_trimspaces(&line[12]);
    }

#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
  else if (strncmp(line, g_groupid, strlen(g_groupid)) == 0)
    {
      /* Save the Group ID */

      status->td_groupid = nsh_trimspaces(&line[12]);
    }
#else
  else if (strncmp(line, g_ppid, strlen(g_ppid)) == 0)
    {
      /* Save the parent thread id */

      status->td_ppid = nsh_trimspaces(&line[12]);
    }
#endif
#endif

#ifdef CONFIG_SMP
  else if (strncmp(line, g_cpu, strlen(g_cpu)) == 0)
    {
      /* Save the current CPU */

      status->td_cpu = nsh_trimspaces(&line[12]);
    }
#endif

  else if (strncmp(line, g_state, strlen(g_state)) == 0)
    {
      FAR char *ptr;

      /* Save the thread state */

      status->td_state = nsh_trimspaces(&line[12]);

      /* Check if an event follows the state */

      ptr = strchr(status->td_state, ',');
      if (ptr != NULL)
        {
          *ptr++ = '\0';
          status->td_event = nsh_trimspaces(ptr);
        }
    }
  else if (strncmp(line, g_flags, strlen(g_flags)) == 0)
    {
      status->td_flags = nsh_trimspaces(&line[12]);
    }
  else if (strncmp(line, g_priority, strlen(g_priority)) == 0)
    {
      FAR char *ptr = nsh_trimspaces(&line[12]);
      status->td_priority = ptr;

      /* If priority inheritance is enabled, use current pri, ignore base */

      while (isdigit(*ptr))
        {
          ++ptr;
        }

      *ptr = '\0';
    }
  else if (strncmp(line, g_scheduler, strlen(g_scheduler)) == 0)
    {
      /* Skip over the SCHED_ part of the policy.  Resultis max 8 bytes */

      status->td_policy = nsh_trimspaces(&line[12+6]);
    }

#ifndef CONFIG_DISABLE_SIGNALS
  else if (strncmp(line, g_sigmask, strlen(g_sigmask)) == 0)
    {
      status->td_sigmask = nsh_trimspaces(&line[12]);
    }
#endif
}
#endif

/****************************************************************************
 * Name: ps_callback
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
static int ps_callback(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                       FAR struct dirent *entryp, FAR void *pvarg)

{
  struct nsh_taskstatus_s status;
  FAR char *filepath;
  FAR char *line;
  FAR char *nextline;
  int ret;
  int i;
#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
  uint32_t stack_size;
#ifdef CONFIG_STACK_COLORATION
  uint32_t stack_used;
  uint32_t stack_filled;
#endif
#endif

  /* Task/thread entries in the /proc directory will all be (1) directories
   * with (2) all numeric names.
   */

  if (!DIRENT_ISDIRECTORY(entryp->d_type))
    {
      /* Not a directory... skip this entry */

      return OK;
    }

  /* Check each character in the name */

  for (i = 0; i < NAME_MAX && entryp->d_name[i] != '\0'; i++)
    {
      if (!isdigit(entryp->d_name[i]))
        {
          /* Name contains something other than a numeric character */

          return OK;
        }
    }

  /* Set all pointers to the empty string. */

  status.td_type     = "";
#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
  status.td_groupid  = "";
#else
  status.td_ppid     = "";
#endif
#endif
#ifdef CONFIG_SMP
  status.td_cpu      = "";
#endif
  status.td_state    = "";
  status.td_event    = "";
  status.td_flags    = "";
  status.td_priority = "";
  status.td_policy   = "";
#ifndef CONFIG_DISABLE_SIGNALS
  status.td_sigmask  = "";
#endif

  /* Read the task status */

  filepath = NULL;
  ret = asprintf(&filepath, "%s/%s/status", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_output(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
    }
  else
    {
      ret = nsh_readfile(vtbl, "ps", filepath, vtbl->iobuffer, IOBUFFERSIZE);
      free(filepath);

      if (ret >= 0)
        {
          /* Parse the task status. */

          nextline = vtbl->iobuffer;
          do
            {
              /* Find the beginning of the next line and NUL-terminat the
               * current line.
               */

              line = nextline;
              for (nextline++;
                   *nextline != '\n' && *nextline != '\0';
                   nextline++);

              if (*nextline == '\n')
                {
                  *nextline++ = '\0';
                }
              else
                {
                  nextline = NULL;
                }

              /* Parse the current line */

              nsh_parse_statusline(line, &status);
            }
          while (nextline != NULL);
        }
    }

  /* Finally, print the status information */

  nsh_output(vtbl, "%5s ", entryp->d_name);

#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
  nsh_output(vtbl, "%5s ", status.td_groupid);
#else
  nsh_output(vtbl, "%5s ", status.td_ppid);
#endif
#endif

#ifdef CONFIG_SMP
  nsh_output(vtbl, "%3s ", status.td_cpu);
#endif

  nsh_output(vtbl, "%3s %-8s %-7s %3s %-8s %-9s ",
             status.td_priority, status.td_policy, status.td_type,
             status.td_flags, status.td_state, status.td_event);

#ifndef CONFIG_DISABLE_SIGNALS
  nsh_output(vtbl, "%-8s ", status.td_sigmask);
#endif

#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
  /* Get the StackSize and StackUsed */

  stack_size = 0;
#ifdef CONFIG_STACK_COLORATION
  stack_used = 0;
#endif
  filepath   = NULL;

  ret = asprintf(&filepath, "%s/%s/stack", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_output(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
      vtbl->iobuffer[0] = '\0';
    }
  else
    {
      ret = nsh_readfile(vtbl, "ps", filepath, vtbl->iobuffer,
                         IOBUFFERSIZE);
      free(filepath);

      if (ret >= 0)
        {
          nextline = vtbl->iobuffer;
          do
            {
              /* Find the beginning of the next line and NUL-terminate the
               * current line.
               */

              line = nextline;
              for (nextline++;
                  *nextline != '\n' && *nextline != '\0';
                  nextline++);

              if (*nextline == '\n')
                {
                  *nextline++ = '\0';
                }
              else
                {
                  nextline = NULL;
                }

              /* Parse the current line
               *
               *   Format:
               *
               *            111111111122222222223
               *   123456789012345678901234567890
               *   StackBase:  xxxxxxxxxx
               *   StackSize:  xxxx
               *   StackUsed:  xxxx
               */

              if (strncmp(line, g_stacksize, strlen(g_stacksize)) == 0)
                {
                  stack_size = (uint32_t)atoi(&line[12]);
                }
#ifdef CONFIG_STACK_COLORATION
              else if (strncmp(line, g_stackused, strlen(g_stackused)) == 0)
                {
                  stack_used = (uint32_t)atoi(&line[12]);
                }
#endif
            }
          while (nextline != NULL);
        }
    }

  nsh_output(vtbl, "%06u ", (unsigned int)stack_size);

#ifdef CONFIG_STACK_COLORATION
  nsh_output(vtbl, "%06u ", (unsigned int)stack_used);

  stack_filled = 0;
  if (stack_size > 0 && stack_used > 0)
    {
      /* Use fixed-point math with one decimal place */

      stack_filled = 10 * 100 * stack_used / stack_size;
    }

  /* Additionally print a '!' if the stack is filled more than 80% */

  nsh_output(vtbl, "%3d.%1d%%%c ",
             stack_filled / 10, stack_filled % 10,
             (stack_filled >= 10 * 80 ? '!' : ' '));
#endif
#endif

#ifdef NSH_HAVE_CPULOAD
  /* Get the CPU load */

  filepath          = NULL;
  ret = asprintf(&filepath, "%s/%s/loadavg", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_output(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
      vtbl->iobuffer[0] = '\0';
    }
  else
    {
      ret = nsh_readfile(vtbl, "ps", filepath, vtbl->iobuffer, IOBUFFERSIZE);
      free(filepath);

      if (ret < 0)
        {
          vtbl->iobuffer[0] = '\0';
        }
    }

  nsh_output(vtbl, "%6s ", nsh_trimspaces(vtbl->iobuffer));
#endif

  /* Read the task/tread command line */

  filepath = NULL;
  ret = asprintf(&filepath, "%s/%s/cmdline", dirpath, entryp->d_name);

  if (ret < 0 || filepath == NULL)
    {
      nsh_output(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
      return ERROR;
    }

  ret = nsh_readfile(vtbl, "ps", filepath, vtbl->iobuffer, IOBUFFERSIZE);
  free(filepath);

  if (ret < 0)
    {
      return ERROR;
    }

  nsh_output(vtbl, "%s\n", nsh_trimspaces(vtbl->iobuffer));
  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_exec
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_EXEC
int cmd_exec(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *endptr;
  uintptr_t addr;

  addr = (uintptr_t)strtol(argv[1], &endptr, 0);
  if (!addr || endptr == argv[1] || *endptr != '\0')
    {
       nsh_output(vtbl, g_fmtarginvalid, argv[0]);
       return ERROR;
    }

  nsh_output(vtbl, "Calling %p\n", (exec_t)addr);
  return ((exec_t)addr)();
}
#endif

/****************************************************************************
 * Name: cmd_ps
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
int cmd_ps(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  nsh_output(vtbl, "%5s ", "PID");

#ifdef CONFIG_SCHED_HAVE_PARENT
#ifdef HAVE_GROUPID
  nsh_output(vtbl, "%5s ", "GROUP");
#else
  nsh_output(vtbl, "%5s ", "PPID");
#endif
#endif

#ifdef CONFIG_SMP
  nsh_output(vtbl, "%3s ", "CPU");
#endif

  nsh_output(vtbl, "%3s %-8s %-7s %3s %-8s %-9s ",
             "PRI", "POLICY", "TYPE", "NPX", "STATE", "EVENT");

#ifndef CONFIG_DISABLE_SIGNALS
  nsh_output(vtbl, "%-8s ", "SIGMASK");
#endif

#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
  nsh_output(vtbl, "%6s ", "STACK");
#ifdef CONFIG_STACK_COLORATION
  nsh_output(vtbl, "%6s ", "USED");
  nsh_output(vtbl, "%7s ", "FILLED");
#endif
#endif

#ifdef NSH_HAVE_CPULOAD
  nsh_output(vtbl, "%6s ", "CPU");
#endif
  nsh_output(vtbl, "%s\n", "COMMAND");

  return nsh_foreach_direntry(vtbl, "ps", CONFIG_NSH_PROC_MOUNTPOINT,
                              ps_callback, NULL);
}
#endif

/****************************************************************************
 * Name: cmd_kill
 ****************************************************************************/

#ifndef CONFIG_DISABLE_SIGNALS
#ifndef CONFIG_NSH_DISABLE_KILL
int cmd_kill(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  char *ptr;
  char *endptr;
  long signal;
  long pid;

  /* Check incoming parameters.  The first parameter should be "-<signal>" */

  ptr = argv[1];
  if (*ptr != '-' || ptr[1] < '0' || ptr[1] > '9')
    {
      goto invalid_arg;
    }

  /* Extract the signal number */

  signal = strtol(&ptr[1], &endptr, 0);

  /* The second parameter should be <pid>  */

  ptr = argv[2];
  if (*ptr < '0' || *ptr > '9')
    {
      goto invalid_arg;
    }

  /* Extract athe pid */

  pid = strtol(ptr, &endptr, 0);

  /* Send the signal.  Kill return values:
   *
   *   EINVAL An invalid signal was specified.
   *   EPERM  The process does not have permission to send the signal to any
   *          of the target processes.
   *   ESRCH  The pid or process group does not exist.
   *   ENOSYS Do not support sending signals to process groups.
   */

  if (kill((pid_t)pid, (int)signal) == 0)
    {
      return OK;
    }

  switch (errno)
    {
    case EINVAL:
      goto invalid_arg;

    case ESRCH:
      nsh_output(vtbl, g_fmtnosuch, argv[0], "task", argv[2]);
      return ERROR;

    case EPERM:
    case ENOSYS:
    default:
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "kill", NSH_ERRNO);
      return ERROR;
    }

invalid_arg:
  nsh_output(vtbl, g_fmtarginvalid, argv[0]);
  return ERROR;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_sleep
 ****************************************************************************/

#ifndef CONFIG_DISABLE_SIGNALS
#ifndef CONFIG_NSH_DISABLE_SLEEP
int cmd_sleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  char *endptr;
  long secs;

  secs = strtol(argv[1], &endptr, 0);
  if (!secs || endptr == argv[1] || *endptr != '\0')
    {
       nsh_output(vtbl, g_fmtarginvalid, argv[0]);
       return ERROR;
    }

  sleep(secs);
  return OK;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_usleep
 ****************************************************************************/

#ifndef CONFIG_DISABLE_SIGNALS
#ifndef CONFIG_NSH_DISABLE_USLEEP
int cmd_usleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  char *endptr;
  long usecs;

  usecs = strtol(argv[1], &endptr, 0);
  if (!usecs || endptr == argv[1] || *endptr != '\0')
    {
       nsh_output(vtbl, g_fmtarginvalid, argv[0]);
       return ERROR;
    }

  usleep(usecs);
  return OK;
}
#endif
#endif
