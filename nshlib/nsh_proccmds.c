/****************************************************************************
 * apps/nshlib/nsh_proccmds.c
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
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <sys/param.h>
#include <time.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_NSH_PROC_MOUNTPOINT
#  define CONFIG_NSH_PROC_MOUNTPOINT "/proc"
#endif

#ifndef CONFIG_NSH_DISABLE_UPTIME
#  ifndef FSHIFT
#    define FSHIFT SI_LOAD_SHIFT
#  endif
#  define FIXED_1      (1 << FSHIFT)     /* 1.0 as fixed-point */
#  define LOAD_INT(x)  ((x) >> FSHIFT)
#  define LOAD_FRAC(x) (LOAD_INT(((x) & (FIXED_1 - 1)) * 100))
#endif

#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
#  define PS_SHOW_HEAPSIZE
#endif

#ifndef CONFIG_NSH_DISABLE_PSSTACKUSAGE
#  define PS_SHOW_STACKSIZE
#  ifdef CONFIG_STACK_COLORATION
#    define PS_SHOW_STACKUSAGE
#  endif
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
  FAR const char *td_groupid;    /* Group ID */
#ifdef CONFIG_SMP
  FAR const char *td_cpu;        /* CPU */
#endif
  FAR const char *td_state;      /* Thread state */
  FAR const char *td_event;      /* Thread wait event */
  FAR const char *td_flags;      /* Thread flags */
  FAR const char *td_priority;   /* Thread priority */
  FAR const char *td_policy;     /* Thread scheduler */
  FAR const char *td_sigmask;    /* Signal mask */
};

/* Status strings */

#ifndef CONFIG_NSH_DISABLE_PS
#if 0 /* Not used */
static const char g_name[]      = "Name:";
#endif

static const char g_type[]      = "Type:";
static const char g_groupid[]   = "Group:";

#ifdef CONFIG_SMP
static const char g_cpu[]       = "CPU:";
#endif

static const char g_state[]     = "State:";
static const char g_flags[]     = "Flags:";
static const char g_priority[]  = "Priority:";
static const char g_scheduler[] = "Scheduler:";
static const char g_sigmask[]   = "SigMask:";

#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
static const char g_heapsize[]  = "AllocSize:";
#endif /* CONFIG_DEBUG _MM && !CONFIG_NSH_DISABLE_PSHEAPUSAGE */

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
   *   Name:       xxxx...            Task/thread name
   *                                  (See CONFIG_TASK_NAME_SIZE)
   *   Type:       xxxxxxx            {Task, pthread, Kthread, Invalid}
   *   Type:       xxxxxxx            {Task, pthread, Kthread, Invalid}
   *   PPID:       xxxxx              Parent thread ID
   *   State:      xxxxxxxx,xxxxxxxxx {Invalid, Waiting, Ready, Running,
   *                                  Inactive}, {Unlock, Semaphore, Signal,
   *                                   MQ empty, MQ full}
   *   Flags:      xxx                N,P,X
   *   Priority:   nnn                Decimal, 0-255
   *   Scheduler:  xxxxxxxxxxxxxx     {SCHED_FIFO, SCHED_RR, SCHED_SPORADIC,
   *                                  SCHED_OTHER}
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
  else if (strncmp(line, g_groupid, strlen(g_groupid)) == 0)
    {
      /* Save the Group ID */

      status->td_groupid = nsh_trimspaces(&line[12]);
    }

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
      /* Skip over the SCHED_ part of the policy.  Result is max 8 bytes. */

      status->td_policy = nsh_trimspaces(&line[12 + 6]);
    }
  else if (strncmp(line, g_sigmask, strlen(g_sigmask)) == 0)
    {
      status->td_sigmask = nsh_trimspaces(&line[12]);
    }
}
#endif

/****************************************************************************
 * Name: ps_callback
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
static int ps_callback(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                       FAR struct dirent *entryp, FAR void *pvarg)
{
  UNUSED(pvarg);

  struct nsh_taskstatus_s status;
  FAR char *filepath;
  FAR char *line;
  FAR char *nextline;
  int ret;
  int i;
#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
  unsigned long heap_size = 0;
#endif
#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
  unsigned long stack_size = 0;
#ifdef CONFIG_STACK_COLORATION
  unsigned long stack_used = 0;
  unsigned long stack_filled = 0;
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
  status.td_groupid  = "";
#ifdef CONFIG_SMP
  status.td_cpu      = "";
#endif
  status.td_state    = "";
  status.td_event    = "";
  status.td_flags    = "";
  status.td_priority = "";
  status.td_policy   = "";
  status.td_sigmask  = "";

  /* Read the task status */

  filepath = NULL;
  ret = asprintf(&filepath, "%s/%s/status", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
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

              /* Parse the current line */

              nsh_parse_statusline(line, &status);
            }
          while (nextline != NULL);
        }
    }

  /* Finally, print the status information */

  nsh_output(vtbl,
             "%5s %5s "
#ifdef CONFIG_SMP
             "%3s "
#endif

             "%3s %-8s %-7s %3s %-8s %-9s %-8s ",
             entryp->d_name, status.td_groupid,
#ifdef CONFIG_SMP
             status.td_cpu,
#endif
             status.td_priority, status.td_policy, status.td_type,
             status.td_flags, status.td_state, status.td_event,
             status.td_sigmask);

#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
  /* Get the Heap AllocSize */

  filepath  = NULL;
  ret = asprintf(&filepath, "%s/%s/heap", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
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
               *   AllocSize:  xxxx
               *   AllocBlks:  xxxx
               */

              if (strncmp(line, g_heapsize, strlen(g_heapsize)) == 0)
                {
                  heap_size = strtoul(&line[12], NULL, 0);
                  break;
                }
            }
          while (nextline != NULL);
        }
    }

#endif

#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
  /* Get the StackSize and StackUsed */

  filepath   = NULL;
  ret = asprintf(&filepath, "%s/%s/stack", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
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
                  stack_size = strtoul(&line[12], NULL, 0);
                }
#ifdef CONFIG_STACK_COLORATION
              else if (strncmp(line, g_stackused, strlen(g_stackused)) == 0)
                {
                  stack_used = strtoul(&line[12], NULL, 0);
                }
#endif
            }
          while (nextline != NULL);
        }
    }

#ifdef CONFIG_STACK_COLORATION

  if (stack_size > 0 && stack_used > 0)
    {
      /* Use fixed-point math with one decimal place */

      stack_filled = 10 * 100 * stack_used / stack_size;
    }
#endif
#endif

#ifdef NSH_HAVE_CPULOAD
  /* Get the CPU load */

  filepath          = NULL;
  ret = asprintf(&filepath, "%s/%s/loadavg", dirpath, entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
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
#endif

#if defined(PS_SHOW_HEAPSIZE) || defined (PS_SHOW_STACKSIZE) || \
    defined (PS_SHOW_STACKUSAGE) || defined (NSH_HAVE_CPULOAD)
    nsh_output(vtbl,
#ifdef PS_SHOW_HEAPSIZE
               "%08lu "
#endif
#ifdef PS_SHOW_STACKSIZE
               "%06lu "
#endif
#ifdef PS_SHOW_STACKUSAGE
               "%06lu "
               "%3lu.%lu%%%c "
#endif
#ifdef NSH_HAVE_CPULOAD
               "%6s "
#endif
#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
               , heap_size
#endif
#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
               , stack_size
#endif
#ifdef PS_SHOW_STACKUSAGE
               , stack_used,
               stack_filled / 10,
               stack_filled % 10,
               (stack_filled >= 10 * 80 ? '!' : ' ')
#endif
#ifdef NSH_HAVE_CPULOAD
               , nsh_trimspaces(vtbl->iobuffer)
#endif
             );
#endif

  /* Read the task/thread command line */

  filepath = NULL;
  ret = asprintf(&filepath, "%s/%s/cmdline", dirpath, entryp->d_name);

  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
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
  UNUSED(argc);

  FAR char *endptr;
  uintptr_t addr;

  addr = (uintptr_t)strtol(argv[1], &endptr, 0);
  if (!addr || endptr == argv[1] || *endptr != '\0')
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  nsh_output(vtbl, "Calling %p\n", (void *)addr);
  return ((exec_t)addr)();
}
#endif

/****************************************************************************
 * Name: cmd_ps
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
int cmd_ps(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);
  UNUSED(argv);

  nsh_output(vtbl, "%5s %5s "
#ifdef CONFIG_SMP
                   "%3s "
#endif
                   "%3s %-8s %-7s %3s %-8s %-9s "
                   "%-16s "
#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
                   "%8s "
#endif
#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
                   "%6s "
#ifdef CONFIG_STACK_COLORATION
                   "%6s "
                   "%7s "
#endif
#endif
#ifdef NSH_HAVE_CPULOAD
                    "%6s "
#endif
                    "%s\n",
                    "PID", "GROUP",
#ifdef CONFIG_SMP
                    "CPU",
#endif
                    "PRI", "POLICY", "TYPE", "NPX", "STATE", "EVENT",
                    "SIGMASK",
#if CONFIG_MM_BACKTRACE >= 0 && !defined(CONFIG_NSH_DISABLE_PSHEAPUSAGE)
                    "HEAP",
#endif
#if !defined(CONFIG_NSH_DISABLE_PSSTACKUSAGE)
                    "STACK",
#ifdef CONFIG_STACK_COLORATION
                    "USED",
                    "FILLED",
#endif
#endif
#ifdef NSH_HAVE_CPULOAD
                    "CPU",
#endif
                    "COMMAND"
                    );

  return nsh_foreach_direntry(vtbl, "ps", CONFIG_NSH_PROC_MOUNTPOINT,
                              ps_callback, NULL);
}
#endif

/****************************************************************************
 * Name: cmd_pidof
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_PIDOF)
int cmd_pidof(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR const char *name = argv[argc - 1];
  pid_t pids[8];
  ssize_t ret;
  int i;

  ret = nsh_getpid(vtbl, name, pids, nitems(pids));
  if (ret <= 0)
    {
      nsh_error(vtbl, g_fmtnosuch, argv[0], "task",  name);
      return ERROR;
    }

  for (i = 0; i < ret; i++)
    {
      nsh_output(vtbl, "%d ", pids[i]);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_kill
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_KILL
int cmd_kill(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *ptr;
  FAR char *endptr;
  long signal;
  long pid;

  /* kill will send SIGTERM to the task in case no signal is selected by
   * -<signal> option
   */

  if (argc == 3)  /* kill -<signal> <pid> */
    {
      /* Check incoming parameters.
       * The first parameter should be "-<signal>"
       */

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
    }
  else if (argc == 2)           /* kill <pid> */
    {
      /* uses default signal number as SIGTERM */

      signal = (long) SIGTERM;  /* SIGTERM is always defined in signal.h */

      /* The first parameter should be <pid>  */

      ptr = argv[1];

      if (*ptr < '0' || *ptr > '9')
        {
          goto invalid_arg;
        }
    }
  else
    {
      /* invalid number of arguments */

      goto invalid_arg;
    }

  /* Extract the pid */

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
      nsh_error(vtbl, g_fmtnosuch, argv[0], "task", argv[2]);
      return ERROR;

    case EPERM:
    case ENOSYS:
    default:
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "kill", NSH_ERRNO);
      return ERROR;
    }

invalid_arg:
  nsh_error(vtbl, g_fmtarginvalid, argv[0]);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_sleep
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_SLEEP
int cmd_sleep(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *endptr;
  long secs;

  secs = strtol(argv[1], &endptr, 0);
  if (!secs || endptr == argv[1] || *endptr != '\0')
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  sleep(secs);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_usleep
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_USLEEP
int cmd_usleep(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *endptr;
  long usecs;

  usecs = strtol(argv[1], &endptr, 0);
  if (!usecs || endptr == argv[1] || *endptr != '\0')
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  usleep(usecs);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_uptime
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_UPTIME
int cmd_uptime(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  uint32_t updays;
  uint32_t uphours;
  uint32_t upminutes;

  time_t current_time_seconds;
  FAR struct tm *current_time;

  struct sysinfo sys_info;

  time_t uptime = 0;

  bool pretty_format_opt = false;
  bool system_load_opt = false;

  if (argc < 2)
    {
      system_load_opt = true;

      current_time_seconds = time(NULL);
      current_time = localtime(&current_time_seconds);
      nsh_output(vtbl, "%02u:%02u:%02u ", current_time->tm_hour,
                 current_time->tm_min, current_time->tm_sec);
    }
  else if (strcmp(argv[1], "-p") == 0)
    {
      pretty_format_opt = true;
    }
  else if (strcmp(argv[1], "-s") == 0)
    {
      sysinfo(&sys_info);
      time(&current_time_seconds);
      current_time_seconds -= sys_info.uptime;
      current_time = localtime(&current_time_seconds);
      nsh_output(vtbl, "%04u-%02u-%02u %02u:%02u:%02u\n",
                 current_time->tm_year + 1900, current_time->tm_mon + 1,
                 current_time->tm_mday, current_time->tm_hour,
                 current_time->tm_min, current_time->tm_sec);
      return OK;
    }
  else
    {
      if (strcmp(argv[1], "-h") != 0)
        {
          nsh_output(vtbl, "uptime: invalid option -- %s\n", argv[1]);
        }

      nsh_output(vtbl, "Usage:\n");
      nsh_output(vtbl, "uptime [options]\n");

      nsh_output(vtbl, "Options:\n");
      nsh_output(vtbl, "-p, show uptime in pretty format\n");
      nsh_output(vtbl, "-h, display this help and exit\n");
      nsh_output(vtbl, "-s, system up since\n");

      return ERROR;
    }

  sysinfo(&sys_info);
  uptime = sys_info.uptime;

  updays = uptime / 86400;
  uptime -= updays * 86400;
  uphours = uptime / 3600;
  uptime -= uphours * 3600;
  upminutes = uptime / 60;

  nsh_output(vtbl, "up ");

  if (updays)
    {
      nsh_output(vtbl, "%" PRIu32 " day%s, ", updays,
                 (updays > 1) ? "s" : "");
    }

  if (pretty_format_opt)
    {
      if (uphours)
        {
          nsh_output(vtbl, "%" PRIu32 " hour%s, ", uphours,
                     (uphours > 1) ? "s" : "");
        }

      nsh_output(vtbl, "%" PRIu32 " minute%s", upminutes,
                 (upminutes > 1) ? "s" : "");
    }
  else
    {
      nsh_output(vtbl, "%2" PRIu32 ":" "%02" PRIu32, uphours, upminutes);
    }

  if (system_load_opt)
    {
      nsh_output(vtbl, ", load average: %lu.%02lu, %lu.%02lu, %lu.%02lu",
                 LOAD_INT(sys_info.loads[0]), LOAD_FRAC(sys_info.loads[0]),
                 LOAD_INT(sys_info.loads[1]), LOAD_FRAC(sys_info.loads[1]),
                 LOAD_INT(sys_info.loads[2]), LOAD_FRAC(sys_info.loads[2]));
    }

  nsh_output(vtbl, "\n");

  return OK;
}
#endif
