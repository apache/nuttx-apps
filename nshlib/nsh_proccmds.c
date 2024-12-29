/****************************************************************************
 * apps/nshlib/nsh_proccmds.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
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
  FAR const char *td_type;         /* Thread type */
  FAR const char *td_groupid;      /* Group ID */
#ifdef CONFIG_SMP
  FAR const char *td_cpu;          /* CPU */
#endif
  FAR const char *td_state;        /* Thread state */
  FAR const char *td_event;        /* Thread wait event */
  FAR const char *td_flags;        /* Thread flags */
  FAR const char *td_priority;     /* Thread priority */
  FAR const char *td_policy;       /* Thread scheduler */
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
  FAR const char *td_sigmask;      /* Signal mask */
#endif
  FAR char       *td_cmdline;      /* Command line */
  int             td_pid;          /* Task ID */
#ifdef NSH_HAVE_CPULOAD
  FAR const char *td_cpuload;      /* CPU load */
#endif
#ifdef PS_SHOW_HEAPSIZE
  unsigned long   td_heapsize;     /* Heap size */
#endif
#ifndef CONFIG_NSH_DISABLE_PSSTACKUSAGE
  unsigned long   td_stack_size;   /* Stack size */
#  ifdef CONFIG_STACK_COLORATION
  unsigned long   td_stack_used;   /* Stack used */
  unsigned long   td_stack_filled; /* Stack filled percentage */
#  endif
#endif
  FAR char       *td_buf;          /* Buffer for reading files */
  size_t          td_bufsize;      /* Size of the buffer */
  size_t          td_bufpos;       /* Position in the buffer */
};

struct nsh_topstatus_s
{
  FAR struct nsh_taskstatus_s **status;
  bool heap;
  size_t size;
  size_t index;
};

/* Status strings */

#ifndef CONFIG_NSH_DISABLE_PS
static const char g_type[]      = "Type:";
static const char g_groupid[]   = "Group:";
#  ifdef CONFIG_SMP
static const char g_cpu[]       = "CPU:";
#  endif
static const char g_state[]     = "State:";
static const char g_flags[]     = "Flags:";
static const char g_priority[]  = "Priority:";
static const char g_scheduler[] = "Scheduler:";
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
static const char g_sigmask[]   = "SigMask:";
#endif
#  ifdef PS_SHOW_HEAPSIZE
static const char g_heapsize[]  = "AllocSize:";
#  endif /* PS_SHOW_HEAPSIZE */
#  ifndef CONFIG_NSH_DISABLE_PSSTACKUSAGE
static const char g_stacksize[] = "StackSize:";
#    ifdef CONFIG_STACK_COLORATION
static const char g_stackused[] = "StackUsed:";
#    endif
#  endif /* !CONFIG_NSH_DISABLE_PSSTACKUSAGE */
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
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
  else if (strncmp(line, g_sigmask, strlen(g_sigmask)) == 0)
    {
      status->td_sigmask = nsh_trimspaces(&line[12]);
    }
#endif
}
#endif

/****************************************************************************
 * Name: ps_skipfile
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PS
static bool ps_skipfile(FAR const struct dirent *entryp)
{
  int i;

  /* Task/thread entries in the /proc directory will all be (1) directories
   * with (2) all numeric names.
   */

  if (!DIRENT_ISDIRECTORY(entryp->d_type))
    {
      /* Not a directory... skip this entry */

      return true;
    }

  /* Check each character in the name */

  for (i = 0; i < NAME_MAX && entryp->d_name[i] != '\0'; i++)
    {
      if (!isdigit(entryp->d_name[i]))
        {
          /* Name contains something other than a numeric character */

          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: ps_readprocfs
 ****************************************************************************/

ssize_t ps_readprocfs(FAR struct nsh_vtbl_s *vtbl, FAR const char *basepath,
                      FAR const char *dirpath,
                      FAR const struct dirent *entryp,
                      FAR struct nsh_taskstatus_s *status)
{
  FAR char *filepath = NULL;
  int ret;

  ret = asprintf(&filepath, "%s/%s/%s", dirpath, entryp->d_name, basepath);
  if (ret < 0 || filepath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ps", "asprintf", NSH_ERRNO);
      status->td_buf[status->td_bufpos] = '\0';
    }
  else
    {
      ret = nsh_readfile(vtbl, "ps", filepath,
                         status->td_buf + status->td_bufpos,
                         status->td_bufsize - status->td_bufpos);
      if (ret >= 0)
        {
          ret = strlen(status->td_buf + status->td_bufpos) + 1;
        }

      free(filepath);
    }

  return ret;
}

/****************************************************************************
 * Name: ps_record
 ****************************************************************************/

static int ps_record(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                     FAR const struct dirent *entryp, bool heap,
                     FAR struct nsh_taskstatus_s *status)
{
  FAR char *nextline;
  FAR char *line;
  int ret;

  status->td_type = "";
  status->td_groupid = "";
#ifdef CONFIG_SMP
  status->td_cpu = "";
#endif
  status->td_state = "";
  status->td_event = "";
  status->td_flags = "";
  status->td_priority = "";
  status->td_policy = "";
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
  status->td_sigmask = "";
#endif
  status->td_cmdline = "";
  status->td_pid = atoi(entryp->d_name);
#ifdef NSH_HAVE_CPULOAD
  status->td_cpuload = "";
#endif

  /* Read the task status */

  ret = ps_readprocfs(vtbl, "status", dirpath, entryp, status);
  if (ret >= 0)
    {
      /* Parse the task status. */

      nextline = status->td_buf + status->td_bufpos;
      status->td_bufpos += ret;
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

          nsh_parse_statusline(line, status);
        }
      while (nextline != NULL);
    }

#ifdef PS_SHOW_HEAPSIZE
  if (heap)
    {
      /* Get the Heap AllocSize */

      ret = ps_readprocfs(vtbl, "heap", dirpath, entryp, status);
      if (ret >= 0)
        {
          nextline = status->td_buf + status->td_bufpos;
          do
            {
              /* Find the beginning of the next line and NUL-terminate
               * the current line.
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
                  status->td_heapsize = strtoul(&line[12], NULL, 0);
                  break;
                }
            }
          while (nextline != NULL);
        }
    }
#endif

#ifdef PS_SHOW_STACKSIZE
  /* Get the StackSize and StackUsed */

  ret = ps_readprocfs(vtbl, "stack", dirpath, entryp, status);
  if (ret >= 0)
    {
      nextline = status->td_buf + status->td_bufpos;
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
              status->td_stack_size = strtoul(&line[12], NULL, 0);
            }
#  ifdef PS_SHOW_STACKUSAGE
          else if (strncmp(line, g_stackused, strlen(g_stackused)) == 0)
            {
              status->td_stack_used = strtoul(&line[12], NULL, 0);
            }
#  endif
        }
      while (nextline != NULL);
    }

#  ifdef PS_SHOW_STACKUSAGE
  if (status->td_stack_size > 0 && status->td_stack_used > 0)
    {
      /* Use fixed-point math with one decimal place */

      status->td_stack_filled = 10 * 100 * status->td_stack_used /
                                status->td_stack_size;
    }
#  endif
#endif

#ifdef NSH_HAVE_CPULOAD
  /* Get the CPU load */

  ret = ps_readprocfs(vtbl, "loadavg", dirpath, entryp, status);
  if (ret >= 0)
    {
      status->td_cpuload = nsh_trimspaces(status->td_buf +
                                          status->td_bufpos);
      status->td_bufpos += ret;
    }
#endif

  /* Read the task/thread command line */

  ret = ps_readprocfs(vtbl, "cmdline", dirpath, entryp, status);
  if (ret >= 0)
    {
      status->td_cmdline = nsh_trimspaces(status->td_buf +
                                          status->td_bufpos);
      status->td_bufpos += ret;
    }

  return ret;
}

/****************************************************************************
 * Name: ps_title
 ****************************************************************************/

static void ps_title(FAR struct nsh_vtbl_s *vtbl, bool heap)
{
#ifdef PS_SHOW_HEAPSIZE
  char heapsize[16];

  if (heap)
    {
      snprintf(heapsize, sizeof(heapsize), "%8s", "HEAP");
    }
  else
    {
      heapsize[0] = '\0';
    }
#endif

  nsh_output(vtbl,
             "%5s %5s "
#ifdef CONFIG_SMP
              "%3s "
#endif
              "%3s %-8s %-7s %3s %-8s %-9s "
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
              "%-16s "
#endif
#ifdef PS_SHOW_HEAPSIZE
              "%s "
#endif
#ifdef PS_SHOW_STACKSIZE
              "%7s "
#endif
#ifdef PS_SHOW_STACKUSAGE
              "%7s %6s "
#endif
#ifdef NSH_HAVE_CPULOAD
              "%6s "
#endif
              "%s\n"
              , "PID", "GROUP"
#ifdef CONFIG_SMP
              , "CPU"
#endif
              , "PRI", "POLICY", "TYPE", "NPX", "STATE", "EVENT"
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
              , "SIGMASK"
#endif
#ifdef PS_SHOW_HEAPSIZE
              , heapsize
#endif
#ifdef PS_SHOW_STACKSIZE
              , "STACK"
#endif
#ifdef PS_SHOW_STACKUSAGE
              , "USED", "FILLED"
#endif
#ifdef NSH_HAVE_CPULOAD
              , "CPU"
#endif
              , "COMMAND");
}

/****************************************************************************
 * Name: ps_output
 ****************************************************************************/

static void ps_output(FAR struct nsh_vtbl_s *vtbl, bool heap,
                      FAR const struct nsh_taskstatus_s *status)
{
  /* Finally, print the status information */

#ifdef PS_SHOW_HEAPSIZE
  char heapsize[16];

  if (heap)
    {
      snprintf(heapsize, sizeof(heapsize), "%08lu", status->td_heapsize);
    }
  else
    {
      heapsize[0] = '\0';
    }
#endif

  nsh_output(vtbl,
             "%5d %5s "
#ifdef CONFIG_SMP
             "%3s "
#endif
             "%3s %-8s %-7s %3s %-8s %-9s "
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
             "%-8s "
#endif
#ifdef PS_SHOW_HEAPSIZE
             "%s "
#endif
#ifdef PS_SHOW_STACKSIZE
             "%07lu "
#endif
#ifdef PS_SHOW_STACKUSAGE
             "%07lu %3lu.%lu%%%c "
#endif
#ifdef NSH_HAVE_CPULOAD
             "%5s "
#endif
             "%s\n"
           , status->td_pid, status->td_groupid
#ifdef CONFIG_SMP
           , status->td_cpu
#endif
           , status->td_priority, status->td_policy , status->td_type
           , status->td_flags , status->td_state, status->td_event
#ifndef CONFIG_NSH_DISABLE_PSSIGMASK
           , status->td_sigmask
#endif
#ifdef PS_SHOW_HEAPSIZE
           , heapsize
#endif
#ifndef CONFIG_NSH_DISABLE_PSSTACKUSAGE
           , status->td_stack_size
#endif
#ifdef PS_SHOW_STACKUSAGE
           , status->td_stack_used
           , status->td_stack_filled / 10 , status->td_stack_filled % 10
           , (status->td_stack_filled >= 10 * 80 ? '!' : ' ')
#endif
#ifdef NSH_HAVE_CPULOAD
           , status->td_cpuload
#endif
           , status->td_cmdline);
}

/****************************************************************************
 * Name: ps_callback
 ****************************************************************************/

static int ps_callback(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                       FAR struct dirent *entryp, FAR void *pvarg)
{
  FAR struct nsh_taskstatus_s status;
  bool heap = *(FAR bool *)pvarg;
  int ret;

  if (ps_skipfile(entryp))
    {
      return OK;
    }

  memset(&status, 0, sizeof(status));
  status.td_buf = vtbl->iobuffer;
  status.td_bufsize = IOBUFFERSIZE;

  ret = ps_record(vtbl, dirpath, entryp, heap, &status);
  if (ret < 0)
    {
      return ret;
    }

  ps_output(vtbl, heap, &status);
  return ret;
}
#endif

#if !defined(CONFIG_NSH_DISABLE_TOP) && defined(NSH_HAVE_CPULOAD)

/****************************************************************************
 * Name: top_callback
 ****************************************************************************/

static int top_callback(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                        FAR struct dirent *entryp, FAR void *pvarg)
{
  FAR struct nsh_topstatus_s *topstatus = pvarg;
  FAR struct nsh_taskstatus_s *status;
  int index = topstatus->index;
  int ret;

  if (ps_skipfile(entryp))
    {
      return OK;
    }

  if (topstatus->size == 0)
    {
      topstatus->status = zalloc(sizeof(FAR struct nsh_taskstatus_s *) * 4);
      if (topstatus->status == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "top", "zalloc", NSH_ERRNO);
          return -ENOMEM;
        }

      topstatus->size = 4;
    }
  else if (topstatus->index >= topstatus->size)
    {
      topstatus->status =
        realloc(topstatus->status,
                sizeof(topstatus->status[0]) * topstatus->size * 2);
      if (topstatus->status == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "top", "realloc", NSH_ERRNO);
          return -ENOMEM;
        }

      memset(&topstatus->status[topstatus->index], 0,
             sizeof(topstatus->status[0]) * topstatus->size);
      topstatus->size *= 2;
    }

  if (topstatus->status[index] != NULL)
    {
      status = topstatus->status[index];
      status->td_bufpos = 0;
    }
  else
    {
      status = zalloc(sizeof(struct nsh_taskstatus_s));
      if (status == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "top", "zalloc", NSH_ERRNO);
          return -ENOMEM;
        }

      topstatus->status[index] = status;
      status->td_buf = zalloc(IOBUFFERSIZE);
      if (status->td_buf == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "top", "zalloc", NSH_ERRNO);
          return -ENOMEM;
        }

      status->td_bufsize = IOBUFFERSIZE;
    }

  ret = ps_record(vtbl, dirpath, entryp, topstatus->heap, status);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "top", "ps_record", NSH_ERRNO);
      return ret;
    }

  topstatus->index++;
  return ret;
}

/****************************************************************************
 * Name: top_cmpcpuload
 ****************************************************************************/

static int top_cmpcpuload(FAR const void *item1, FAR const void *item2)
{
  FAR const struct nsh_taskstatus_s *status1 =
    (FAR const struct nsh_taskstatus_s *)(*(FAR uintptr_t *)item1);
  FAR const struct nsh_taskstatus_s *status2 =
    (FAR const struct nsh_taskstatus_s *)(*(FAR uintptr_t *)item2);
  int load1 = atoi(status1->td_cpuload);
  int load2 = atoi(status2->td_cpuload);
  FAR const char *s1;
  FAR const char *s2;

  if (load1 == load2)
    {
      s1 = status1->td_cpuload;
      s2 = status2->td_cpuload;
      while (*s1++ != '.');
      while (*s2++ != '.');
      if (*s2 == *s1)
        {
          return 0;
        }

      return *s2 > *s1 ? 1 : -1;
    }
  else
    {
      return load2 > load1 ? 1 : -1;
    }
}

/****************************************************************************
 * Name: top_exit
 ****************************************************************************/

static void top_exit(int signo, FAR siginfo_t *siginfo, FAR void *context)
{
  *(FAR bool *)siginfo->si_user = true;
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
  bool heap = false;
  int i;

#ifdef PS_SHOW_HEAPSIZE
  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-heap") == 0)
        {
          heap = true;
        }
    }
#endif

  ps_title(vtbl, heap);

  if (argc - heap > 1)
    {
      for (i = 1; i < argc; i++)
        {
          struct dirent entry;
          if (!isdigit(*argv[i]))
            {
              continue;
            }

          entry.d_type = DT_DIR;
          strcpy(entry.d_name, argv[i]);
          ps_callback(vtbl, CONFIG_NSH_PROC_MOUNTPOINT, &entry, &heap);
        }

      return 0;
    }
  else
    {
      return nsh_foreach_direntry(vtbl, "ps", CONFIG_NSH_PROC_MOUNTPOINT,
                                  ps_callback, &heap);
    }
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

  nsh_output(vtbl, "\n");

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

  if (argc == 3) /* kill -<signal> <pid> */
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
  else if (argc == 2) /* kill <pid> */
    {
      /* Uses default signal number as SIGTERM */

      signal = SIGTERM; /* SIGTERM is always defined in signal.h */

      /* The first parameter should be <pid>  */

      ptr = argv[1];

      if (*ptr < '0' || *ptr > '9')
        {
          goto invalid_arg;
        }
    }
  else
    {
      /* Invalid number of arguments */

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
 * Name: cmd_pkill
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_PKILL)
int cmd_pkill(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR const char *name;
  FAR char *ptr;
  pid_t pids[8];
  int signal;
  ssize_t ret;
  int i;

  /* pkill will send SIGTERM to the task in case no signal is selected by
   * -<signal> option
   */

  if (argc == 3)  /* pkill -<signal> <name> */
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

      signal = atoi(&ptr[1]);

      /* The second parameter should be <pid>  */

      name = argv[2];
    }
  else if (argc == 2)           /* kill <pid> */
    {
      /* uses default signal number as SIGTERM */

      signal = SIGTERM;  /* SIGTERM is always defined in signal.h */

      /* The first parameter should be name  */

      name = argv[1];
    }
  else
    {
      /* invalid number of arguments */

      goto invalid_arg;
    }

  ret = nsh_getpid(vtbl, name, pids, nitems(pids));
  if (ret <= 0)
    {
      nsh_error(vtbl, g_fmtnosuch, argv[0], "task",  name);
      return ERROR;
    }

  /* Send the signal.  Kill return values:
   *
   *   EINVAL An invalid signal was specified.
   *   EPERM  The process does not have permission to send the signal to any
   *          of the target processes.
   *   ESRCH  The pid or process group does not exist.
   *   ENOSYS Do not support sending signals to process groups.
   */

  for (i = 0; i < ret; i++)
    {
      if (kill(pids[i], signal) != 0)
        {
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
        }
    }

  return OK;

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

/****************************************************************************
 * Name: cmd_top
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_TOP) && defined(NSH_HAVE_CPULOAD)
int cmd_top(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *pidlist = NULL;
  size_t num = SIZE_MAX;
  size_t i;
  struct sigaction act;
  bool quit = false;
  int delay = 3;
  int ret = 0;
  int tc = 0;
  int opt;
  struct nsh_topstatus_s topstatus =
    {
      0
    };

  while ((opt = getopt(argc, argv, "d:p:n:h")) != ERROR)
    {
      switch (opt)
        {
          case 'd':
            delay = atoi(optarg);
            break;
          case 'p':
            pidlist = optarg;
            break;
          case 'h':
            topstatus.heap = true;
            break;
          case 'n':
            num = atoi(optarg);
            break;

          default:
            nsh_output(vtbl, "Usage: top"
                             "[ -n <num>] [ -d <delay>]"
                             "[ -p <pidlist>] [ -h ]\n");
            return ERROR;
        }
    }

  act.sa_user = &quit;
  act.sa_sigaction = top_exit;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGINT, &act, NULL) < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "top", "sigaction", NSH_ERRNO);
      return ERROR;
    }

  if (vtbl->isctty)
    {
      tc = nsh_ioctl(vtbl, TIOCSCTTY, getpid());
    }

  while (!quit)
    {
      topstatus.index = 0;
      nsh_output(vtbl, "\033[2J\033[1;1H");
      ps_title(vtbl, topstatus.heap);

      if (pidlist)
        {
          FAR char *save = NULL;
          FAR char *pid = strtok_r(pidlist, ",", &save);

          while (pid != NULL && ret >= 0)
            {
              struct dirent entry;
              entry.d_type = DT_DIR;
              strlcpy(entry.d_name, pid, sizeof(entry.d_name));
              ret = top_callback(vtbl, CONFIG_NSH_PROC_MOUNTPOINT,
                                 &entry, &topstatus);
              *--pid = ',';
              pid = strtok_r(NULL, ",", &save);
            }
        }
      else
        {
          ret = nsh_foreach_direntry(vtbl, "top", CONFIG_NSH_PROC_MOUNTPOINT,
                                     top_callback, &topstatus);
        }

      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "top", "nsh_foreach_direntry",
                    NSH_ERRNO);
          break;
        }

      qsort(topstatus.status, topstatus.index,
            sizeof(topstatus.status[0]), top_cmpcpuload);

      for (i = 0; i < MIN(topstatus.index, num); i++)
        {
          ps_output(vtbl, topstatus.heap, topstatus.status[i]);
        }

      if (vtbl->isctty && tc == 0)
        {
          nsh_output(vtbl, "use Ctrl+c' to quit\n");
        }

      sleep(delay);
    }

  if (topstatus.status != NULL)
    {
      for (i = 0; i < topstatus.size; i++)
        {
          if (topstatus.status[i] != NULL)
            {
              free(topstatus.status[i]->td_buf);
              free(topstatus.status[i]);
            }
        }

      free(topstatus.status);
    }

  if (vtbl->isctty && tc == 0)
    {
      nsh_ioctl(vtbl, TIOCNOTTY, 0);
    }

  return ret;
}
#endif
