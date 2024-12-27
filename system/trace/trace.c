/****************************************************************************
 * apps/system/trace/trace.c
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <sys/ioctl.h>
#include <nuttx/note/notectl_driver.h>

#include "trace.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: notectl_enable
 ****************************************************************************/

static bool notectl_enable(FAR const char *name, int flag, int notectlfd)
{
  struct note_filter_named_mode_s mode;
  int oldflag;

  strlcpy(mode.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETMODE, (unsigned long)&mode);

  oldflag = (mode.mode.flag & NOTE_FILTER_MODE_FLAG_ENABLE) != 0;
  if (flag == oldflag)
    {
      /* Already set */

      return false;
    }

  if (flag)
    {
      mode.mode.flag |= NOTE_FILTER_MODE_FLAG_ENABLE;
    }
  else
    {
      mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_ENABLE;
    }

  ioctl(notectlfd, NOTECTL_SETMODE, (unsigned long)&mode);

  return true;
}

/****************************************************************************
 * Name: trace_cmd_start
 ****************************************************************************/

static int trace_cmd_start(FAR const char *name, int index, int argc,
                           FAR char **argv, int notectlfd)
{
  FAR char *endptr;
  int duration = 0;
  bool cont = false;

  /* Usage: trace start [-c][<duration>] */

  if (index < argc)
    {
      if (strcmp(argv[index], "-c") == 0)
        {
          cont = true;
          index++;
        }
    }

  if (index < argc)
    {
      duration = strtoul(argv[index], &endptr, 0);
      if (!duration || endptr == argv[index] || *endptr != '\0')
        {
          fprintf(stderr,
                  "trace start: invalid argument '%s'\n", argv[index]);
          return ERROR;
        }

      index++;
    }

  /* Clear the trace buffer */

  if (!cont)
    {
      trace_dump_clear();
    }

  /* Start tracing */

  notectl_enable(name, true, notectlfd);

  if (duration > 0)
    {
      /* If <duration> is given, stop tracing after specified seconds. */

      sleep(duration);
      notectl_enable(name, false, notectlfd);
    }

  return index;
}

/****************************************************************************
 * Name: trace_cmd_dump
 ****************************************************************************/

#ifdef CONFIG_DRIVERS_NOTERAM
static int trace_cmd_dump(FAR const char *name, int index, int argc,
                          FAR char **argv, int notectlfd)
{
  FAR FILE *out = stdout;
  bool changed = false;
  bool cont = false;
  int ret;

  /* Usage: trace dump [-c][<filename>] */

  if (index < argc)
    {
      if (strcmp(argv[index], "-c") == 0)
        {
          cont = true;
          index++;
        }
    }

  /* If <filename> is '-' or not given, trace dump is displayed
   * to stdout.
   */

  if (index < argc)
    {
      if (strcmp(argv[index], "-") != 0)
        {
          /* If <filename> is given, open the file stream for output. */

          out = fopen(argv[index], "w");
          if (out == NULL)
            {
              fprintf(stderr,
                      "trace dump: cannot open '%s'\n", argv[index]);
              return ERROR;
            }
        }

      index++;
    }

  /* Stop the tracing before dump */

  if (!cont)
    {
      changed = notectl_enable(name, false, notectlfd);
    }

  /* Dump the trace header */

  fputs("# tracer: nop\n#\n", out);

  /* Dump the trace data */

  ret = trace_dump(out);

  if (changed)
    {
      notectl_enable(name, true, notectlfd);
    }

  /* If needed, close the file stream for dump. */

  if (out != stdout)
    {
      fclose(out);
    }

  if (ret < 0)
    {
      fprintf(stderr,
              "trace dump: dump failed\n");
      return ERROR;
    }

  return index;
}
#endif

/****************************************************************************
 * Name: trace_cmd_cmd
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_SYSTEM
static int trace_cmd_cmd(FAR const char *name, int index, int argc,
                         FAR char **argv, int notectlfd)
{
  char command[LINE_MAX];
  bool changed;
  bool cont = false;

  /* Usage: trace cmd [-c] <command> [<args>...] */

  if (index < argc)
    {
      if (strcmp(argv[index], "-c") == 0)
        {
          cont = true;
          index++;
        }
    }

  if (index >= argc)
    {
      /* <command> parameter is mandatory. */

      fprintf(stderr,
              "trace cmd: no argument\n");
      return ERROR;
    }

  command[0] = '\0';
  while (index < argc)
    {
      strlcat(command, argv[index], sizeof(command));
      strlcat(command, " ", sizeof(command));
      index++;
    }

  /* Clear the trace buffer */

  if (!cont)
    {
      trace_dump_clear();
    }

  /* Execute the command with tracing */

  changed = notectl_enable(name, true, notectlfd);

  system(command);

  if (changed)
    {
      notectl_enable(name, false, notectlfd);
    }

  return index;
}
#endif

/****************************************************************************
 * Name: trace_cmd_mode
 ****************************************************************************/

static int trace_cmd_mode(FAR const char *name, int index, int argc,
                          FAR char **argv, int notectlfd)
{
  struct note_filter_named_mode_s mode;
  bool owmode;
  bool enable;
  bool modified = false;
#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
  struct note_filter_named_syscall_s filter_syscall;
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
  struct note_filter_named_irq_s filter_irq;
#endif

#if defined(CONFIG_SCHED_INSTRUMENTATION_SYSCALL) ||\
    defined(CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER)
  int i;
  int count;
#endif

  /* Usage: trace mode [{+|-}{o|s|a|i}...] */

  /* Get current trace mode */

  strlcpy(mode.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETMODE, (unsigned long)&mode);
  owmode = trace_dump_get_overwrite();

  /* Parse the mode setting parameters */

  while (index < argc)
    {
      if (argv[index][0] != '-' && argv[index][0] != '+')
        {
          break;
        }

      enable = (argv[index][0] == '+');

      switch (argv[index][1])
        {
#ifdef CONFIG_DRIVERS_NOTERAM
          case 'o':   /* Overwrite mode */
            owmode = enable;
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
          case 'w':   /* Switch trace */
            if (enable)
              {
                mode.mode.flag |= NOTE_FILTER_MODE_FLAG_SWITCH;
              }
            else
              {
                mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_SWITCH;
              }
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
          case 's':   /* Syscall trace */
            if (enable)
              {
                mode.mode.flag |= NOTE_FILTER_MODE_FLAG_SYSCALL;
              }
            else
              {
                mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_SYSCALL;
              }
            break;

          case 'a':   /* Record syscall arguments */
            if (enable)
              {
                mode.mode.flag |= NOTE_FILTER_MODE_FLAG_SYSCALL_ARGS;
              }
            else
              {
                mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_SYSCALL_ARGS;
              }
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
          case 'i':   /* IRQ trace */
            if (enable)
              {
                mode.mode.flag |= NOTE_FILTER_MODE_FLAG_IRQ;
              }
            else
              {
                mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_IRQ;
              }
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
          case 'd':   /* Dump trace */
            if (enable)
              {
                mode.mode.flag |= NOTE_FILTER_MODE_FLAG_DUMP;
              }
            else
              {
                mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_DUMP;
              }
            break;
#endif

          default:
            fprintf(stderr,
                    "trace mode: invalid option '%s'\n", argv[index]);
            return ERROR;
        }

      index++;
      modified = true;
    }

  if (modified)
    {
      /* Update trace mode */

      ioctl(notectlfd, NOTECTL_SETMODE, (unsigned long)&mode);
      trace_dump_set_overwrite(owmode);

      return index;
    }

  /* If no parameter, display current trace mode setting. */

  printf("Task trace mode(%s):\n", mode.name);
  printf(" Trace                   : %s\n",
         (mode.mode.flag & NOTE_FILTER_MODE_FLAG_ENABLE) ?
          "enabled" : "disabled");

#ifdef CONFIG_DRIVERS_NOTERAM
  printf(" Overwrite               : %s\n",
         owmode ? "on  (+o)" : "off (-o)");
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
  strlcpy(filter_syscall.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETSYSCALLFILTER,
        (unsigned long)&filter_syscall);
  for (count = i = 0; i < SYS_nsyscalls; i++)
    {
      if (NOTE_FILTER_SYSCALLMASK_ISSET(i, &filter_syscall.syscall_mask))
        {
          count++;
        }
    }

  printf(" Syscall trace           : %s\n",
         mode.mode.flag & NOTE_FILTER_MODE_FLAG_SYSCALL ?
          "on  (+s)" : "off (-s)");
  if (mode.mode.flag & NOTE_FILTER_MODE_FLAG_SYSCALL)
    {
      printf("  Filtered Syscalls      : %d\n", count);
    }

  printf(" Syscall trace with args : %s\n",
         mode.mode.flag & NOTE_FILTER_MODE_FLAG_SYSCALL_ARGS ?
          "on  (+a)" : "off (-a)");
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
  strlcpy(filter_irq.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETIRQFILTER,
        (unsigned long)&filter_irq.irq_mask);
  for (count = i = 0; i < NR_IRQS; i++)
    {
      if (NOTE_FILTER_IRQMASK_ISSET(i, &filter_irq.irq_mask))
        {
          count++;
        }
    }

  printf(" IRQ trace               : %s\n",
         mode.mode.flag & NOTE_FILTER_MODE_FLAG_IRQ ?
          "on  (+i)" : "off (-i)");
  if (mode.mode.flag & NOTE_FILTER_MODE_FLAG_IRQ)
    {
      printf("  Filtered IRQs          : %d\n", count);
    }
#endif

  return index;
}

/****************************************************************************
 * Name: trace_cmd_switch
 ****************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
static int trace_cmd_switch(FAR const char *name, int index, int argc,
                            FAR char **argv, int notectlfd)
{
  bool enable;
  struct note_filter_named_mode_s mode;

  /* Usage: trace switch [+|-] */

  /* Get current filter setting */

  strlcpy(mode.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETMODE, (unsigned long)&mode);

  /* Parse the setting parameters */

  if (index < argc)
    {
      if (argv[index][0] == '-' || argv[index][0] == '+')
        {
          enable = (argv[index++][0] == '+');
          if (enable ==
              ((mode.mode.flag & NOTE_FILTER_MODE_FLAG_SWITCH) != 0))
            {
              /* Already set */

              return index;
            }

          if (enable)
            {
              mode.mode.flag |= NOTE_FILTER_MODE_FLAG_SWITCH;
            }
          else
            {
              mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_SWITCH;
            }

          ioctl(notectlfd, NOTECTL_SETMODE, (unsigned long)&mode);
        }
    }

  return index;
}
#endif

/****************************************************************************
 * Name: trace_cmd_syscall
 ****************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
static int trace_cmd_syscall(FAR const char *name, int index, int argc,
                             FAR char **argv, int notectlfd)
{
  bool enable;
  bool modified = false;
  int syscallno;
  FAR struct note_filter_named_syscall_s filter_syscall;
  int n;
  int count;

  /* Usage: trace syscall [{+|-}<syscallname>...] */

  /* Get current syscall filter setting */

  strlcpy(filter_syscall.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETSYSCALLFILTER,
        (unsigned long)&filter_syscall);

  /* Parse the setting parameters */

  while (index < argc)
    {
      if (argv[index][0] != '-' && argv[index][0] != '+')
        {
          break;
        }

      enable = (argv[index][0] == '+');

      /* Check whether the given pattern matches for each syscall names */

      for (syscallno = 0; syscallno < SYS_nsyscalls; syscallno++)
        {
          if (fnmatch(&argv[index][1], g_funcnames[syscallno], 0))
            {
              continue;
            }

          /* If matches, update the masked syscall number list */

          if (enable)
            {
              NOTE_FILTER_SYSCALLMASK_SET(syscallno,
                                          &filter_syscall.syscall_mask);
            }
          else
            {
              NOTE_FILTER_SYSCALLMASK_CLR(syscallno,
                                          &filter_syscall.syscall_mask);
            }
        }

      index++;
      modified = true;
    }

  if (modified)
    {
      /* Update current syscall filter setting */

      ioctl(notectlfd, NOTECTL_SETSYSCALLFILTER,
            (unsigned long)&filter_syscall);
    }
  else
    {
      /* If no parameter, display current setting. */

      for (count = n = 0; n < SYS_nsyscalls; n++)
        {
          if (NOTE_FILTER_SYSCALLMASK_ISSET(n, &filter_syscall.syscall_mask))
            {
              count++;
            }
        }

      printf("Filtered Syscalls: %d\n", count);

      for (n = 0; n < SYS_nsyscalls; n++)
        {
          if (NOTE_FILTER_SYSCALLMASK_ISSET(n, &filter_syscall.syscall_mask))
            {
              printf("  %s\n", g_funcnames[n]);
            }
        }
    }

  return index;
}
#endif

/****************************************************************************
 * Name: trace_cmd_irq
 ****************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
static int trace_cmd_irq(FAR const char *name, int index, int argc,
                         FAR char **argv, int notectlfd)
{
  bool enable;
  bool modified = false;
  int irqno;
  FAR char *endptr;
  struct note_filter_named_irq_s filter_irq;
  int n;
  int count;

  /* Usage: trace irq [{+|-}<irqnum>...] */

  /* Get current irq filter setting */

  strlcpy(filter_irq.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETIRQFILTER, (unsigned long)&filter_irq);

  /* Parse the setting parameters */

  while (index < argc)
    {
      if (argv[index][0] != '-' && argv[index][0] != '+')
        {
          break;
        }

      enable = (argv[index][0] == '+');

      if (argv[index][1] == '*')
        {
          /* Mask or unmask all IRQs */

          if (enable)
            {
              for (n = 0; n < NR_IRQS; n++)
                {
                  NOTE_FILTER_IRQMASK_SET(n, &filter_irq.irq_mask);
                }
            }
          else
            {
              NOTE_FILTER_IRQMASK_ZERO(&filter_irq.irq_mask);
            }
        }
      else
        {
          /* Get IRQ number */

          irqno = strtoul(&argv[index][1], &endptr, 0);
          if (endptr == &argv[index][1] || *endptr != '\0' ||
              irqno >= NR_IRQS)
            {
              fprintf(stderr,
                      "trace irq: invalid argument '%s'\n", argv[index]);
              return ERROR;
            }

          /* Update the masked IRQ number list */

          if (enable)
            {
              NOTE_FILTER_IRQMASK_SET(irqno, &filter_irq.irq_mask);
            }
          else
            {
              NOTE_FILTER_IRQMASK_CLR(irqno, &filter_irq.irq_mask);
            }
        }

      index++;
      modified = true;
    }

  if (modified)
    {
      /* Update current irq filter setting */

      ioctl(notectlfd, NOTECTL_SETIRQFILTER, (unsigned long)&filter_irq);
    }
  else
    {
      /* If no parameter, display current setting. */

      for (count = n = 0; n < NR_IRQS; n++)
        {
          if (NOTE_FILTER_IRQMASK_ISSET(n, &filter_irq.irq_mask))
            {
              count++;
            }
        }

      printf("Filtered IRQs: %d\n", count);

      for (n = 0; n < NR_IRQS; n++)
        {
          if (NOTE_FILTER_IRQMASK_ISSET(n, &filter_irq.irq_mask))
            {
              printf("  %d\n", n);
            }
        }
    }

  return index;
}
#endif

/****************************************************************************
 * Name: trace_cmd_print
 ****************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
static int trace_cmd_print(FAR const char *name, int index, int argc,
                           FAR char **argv, int notectlfd)
{
  bool enable;
  struct note_filter_named_mode_s mode;

  /* Usage: trace print [+|-] */

  /* Get current filter setting */

  strlcpy(mode.name, name, NAME_MAX);
  ioctl(notectlfd, NOTECTL_GETMODE, (unsigned long)&mode);

  /* Parse the setting parameters */

  if (index < argc)
    {
      if (argv[index][0] == '-' || argv[index][0] == '+')
        {
          enable = (argv[index++][0] == '+');
          if (enable ==
              ((mode.mode.flag & NOTE_FILTER_MODE_FLAG_DUMP) != 0))
            {
              /* Already set */

              return index;
            }

          if (enable)
            {
              mode.mode.flag |= NOTE_FILTER_MODE_FLAG_DUMP;
            }
          else
            {
              mode.mode.flag &= ~NOTE_FILTER_MODE_FLAG_DUMP;
            }

          ioctl(notectlfd, NOTECTL_SETMODE, (unsigned long)&mode);
        }
    }

  return index;
}
#endif

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(void)
{
  fprintf(stderr,
          "\nUsage: trace [-n][<name>] <subcommand>...\n"
          "Subcommand:\n"
          " start   [-c][<duration>]            :"
                                " Start task tracing\n"
          " stop                                :"
                                " Stop task tracing\n"
#ifdef CONFIG_SYSTEM_SYSTEM
          " cmd     [-c] <command> [<args>...]  :"
                                " Get the trace while running <command>\n"
#endif
#ifdef CONFIG_DRIVERS_NOTERAM
          " dump    [-a][-c][<filename>]        :"
                                " Output the trace result\n"
          "                                       [-a] <Android SysTrace>\n"
#endif
          " mode    [{+|-}{o|w|s|a|i|d}...]     :"
                                " Set task trace options\n"
#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
          " switch  [+|-]                       :"
                                " Configure switch trace filter\n"
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
          " syscall [{+|-}<syscallname>...]     :"
                                " Configure syscall trace filter\n"
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
          " irq     [{+|-}<irqnum>...]          :"
                                " Configure IRQ trace filter\n"
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
          " print   [+|-]                       :"
                                " Configure dump trace filter\n"
#endif
         );

  fflush(stderr);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int notectlfd;
  int exitcode = EXIT_FAILURE;
  int i;
  char name[NAME_MAX];

  /* Open note control device */

  notectlfd = open("/dev/notectl", 0);
  if (notectlfd < 0)
    {
      fprintf(stderr,
              "trace: cannot open /dev/notectl\n");
      goto errout;
    }

  name[0] = '\0';
  if (argc == 1)
    {
      /* No arguments - show current mode */

      trace_cmd_mode(name, 0, 0, NULL, notectlfd);
      goto exit_with_close;
    }

  /* Parse command line arguments */

  i = 1;
  if (strcmp(argv[i], "-n") == 0)
    {
      strlcpy(name, argv[++i], NAME_MAX);
      i++;
    }

  while (i < argc)
    {
      if (strcmp(argv[i], "start") == 0)
        {
          i = trace_cmd_start(name, i + 1, argc, argv, notectlfd);
        }
      else if (strcmp(argv[i], "stop") == 0)
        {
          i++;
          notectl_enable(name, false, notectlfd);
        }
#ifdef CONFIG_DRIVERS_NOTERAM
      else if (strcmp(argv[i], "dump") == 0)
        {
          i = trace_cmd_dump(name, i + 1, argc, argv, notectlfd);
        }
#endif
#ifdef CONFIG_SYSTEM_SYSTEM
      else if (strcmp(argv[i], "cmd") == 0)
        {
          i = trace_cmd_cmd(name, i + 1, argc, argv, notectlfd);
        }
#endif
      else if (strcmp(argv[i], "mode") == 0)
        {
          i = trace_cmd_mode(name, i + 1, argc, argv, notectlfd);
        }
#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
      else if (strcmp(argv[i], "switch") == 0)
        {
          i = trace_cmd_switch(name, i + 1, argc, argv, notectlfd);
        }
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
      else if (strcmp(argv[i], "syscall") == 0)
        {
          i = trace_cmd_syscall(name, i + 1, argc, argv, notectlfd);
        }
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
      else if (strcmp(argv[i], "irq") == 0)
        {
          i = trace_cmd_irq(name, i + 1, argc, argv, notectlfd);
        }
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
      else if (strcmp(argv[i], "print") == 0)
        {
          i = trace_cmd_print(name, i + 1, argc, argv, notectlfd);
        }
#endif
      else
        {
          show_usage();
          goto errout_with_close;
        }

      if (i < 0)
        {
          goto errout_with_close;
        }
    }

exit_with_close:
  exitcode = EXIT_SUCCESS;

  /* Close note */

errout_with_close:
  close(notectlfd);

errout:
  return exitcode;
}
