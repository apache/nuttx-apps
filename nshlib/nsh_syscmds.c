/****************************************************************************
 * apps/nshlib/nsh_syscmds.c
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

#include <nuttx/rptun/rptun.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARCH_BOARD_CUSTOM
#  ifndef CONFIG_ARCH_BOARD_CUSTOM_NAME
#    define BOARD_NAME g_unknown
#  else
#    define BOARD_NAME CONFIG_ARCH_BOARD_CUSTOM_NAME
#  endif
#else
#  ifndef CONFIG_ARCH_BOARD
#    define BOARD_NAME g_unknown
#  else
#    define BOARD_NAME CONFIG_ARCH_BOARD
#  endif
#endif

#define UNAME_KERNEL   (1 << 0)
#define UNAME_NODE     (1 << 1)
#define UNAME_RELEASE  (1 << 2)
#define UNAME_VERISON  (1 << 3)
#define UNAME_MACHINE  (1 << 4)
#define UNAME_PLATFORM (1 << 5)
#define UNAME_UNKNOWN  (1 << 6)

#ifdef CONFIG_NET
#  define UNAME_ALL    (UNAME_KERNEL | UNAME_NODE | UNAME_RELEASE | \
                        UNAME_VERISON | UNAME_MACHINE | UNAME_PLATFORM)
#else
#  define UNAME_ALL    (UNAME_KERNEL | UNAME_RELEASE | UNAME_VERISON | \
                        UNAME_MACHINE | UNAME_PLATFORM)
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_UNAME
static const char g_unknown[] = "unknown";
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_shutdown
 ****************************************************************************/

#if (defined(CONFIG_BOARDCTL_POWEROFF) || defined(CONFIG_BOARDCTL_RESET)) && \
    !defined(CONFIG_NSH_DISABLE_SHUTDOWN)

int cmd_shutdown(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
#if defined(CONFIG_BOARDCTL_POWEROFF) && defined(CONFIG_BOARDCTL_RESET)
  /* If both shutdown and reset are supported, then a single option may
   * be provided to select the reset behavior (--reboot).  We know here
   * that argc is either 1 or 2.
   */

  if (argc == 2)
    {
      /* Verify that the single argument is --reboot */

      if (strcmp(argv[1], "--reboot") != 0)
        {
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          return ERROR;
        }

      /* Invoke the BOARDIOC_RESET board control to reset the board.  If
       * the board_reset() function returns, then it was not possible to
       * reset the board due to some constraints.
       */

      boardctl(BOARDIOC_RESET, EXIT_SUCCESS);
    }
  else
    {
      /* Invoke the BOARDIOC_POWEROFF board control to shutdown the board.
       * If the board_power_off function returns, then it was not possible
       * to power-off the* board due to some constraints.
       */

      boardctl(BOARDIOC_POWEROFF, EXIT_SUCCESS);
    }

#elif defined(CONFIG_BOARDCTL_RESET)
  /* Only reset behavior is supported and we already know that exactly one
   * argument has been provided.
   */

  /* Verify that the single argument is --reboot */

  if (strcmp(argv[1], "--reboot") != 0)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  /* Invoke the BOARDIOC_RESET board control to reset the board.  If
   * the board_reset() function returns, then it was not possible to
   * reset the board due to some constraints.
   */

  boardctl(BOARDIOC_RESET, EXIT_SUCCESS);

#else
  /* Only the reset behavior is supported and we already know that there is
   * no argument to the command.
   */

  /* Invoke the BOARDIOC_POWEROFF board control to shutdown the board.  If
   * the board_power_off function returns, then it was not possible to power-
   * off the board due to some constraints.
   */

  boardctl(BOARDIOC_POWEROFF, EXIT_SUCCESS);
#endif

  /* boarctl() will not return in any case.  It if does, it means that
   * there was a problem with the shutdown/resaet operation.
   */

  nsh_error(vtbl, g_fmtcmdfailed, argv[0], "boardctl", NSH_ERRNO);
  return ERROR;
}
#endif /* CONFIG_BOARDCTL_POWEROFF && !CONFIG_NSH_DISABLE_SHUTDOWN */

/****************************************************************************
 * Name: cmd_pmconfig
 ****************************************************************************/

#if defined(CONFIG_PM) && !defined(CONFIG_NSH_DISABLE_PMCONFIG)
int cmd_pmconfig(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct boardioc_pm_ctrl_s ctrl =
  {
  };

  if (argc == 1)
    {
      int current_state;
      int normal_count;
      int idle_count;
      int standby_count;
      int sleep_count;

      ctrl.action = BOARDIOC_PM_QUERYSTATE;
      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
      current_state = ctrl.state;

      ctrl.action = BOARDIOC_PM_STAYCOUNT;
      ctrl.state = PM_NORMAL;
      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
      normal_count = ctrl.count;

      ctrl.state = PM_IDLE;
      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
      idle_count = ctrl.count;

      ctrl.state = PM_STANDBY;
      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
      standby_count = ctrl.count;

      ctrl.state = PM_SLEEP;
      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
      sleep_count = ctrl.count;

      nsh_output(vtbl, "Current state %d, PM stay [%d, %d, %d, %d]\n",
        current_state, normal_count, idle_count, standby_count, sleep_count);
    }
  else if (argc == 3)
    {
      if (strcmp(argv[1], "stay") == 0)
        {
          ctrl.action = BOARDIOC_PM_STAY;
        }
      else if (strcmp(argv[1], "relax") == 0)
        {
          ctrl.action = BOARDIOC_PM_RELAX;
        }
      else
        {
          nsh_output(vtbl, g_fmtarginvalid, argv[1]);
          return ERROR;
        }

      if (strcmp(argv[2], "normal") == 0)
        {
          ctrl.state = PM_NORMAL;
        }
      else if (strcmp(argv[2], "idle") == 0)
        {
          ctrl.state = PM_IDLE;
        }
      else if (strcmp(argv[2], "standby") == 0)
        {
          ctrl.state = PM_STANDBY;
        }
      else if (strcmp(argv[2], "sleep") == 0)
        {
          ctrl.state = PM_SLEEP;
        }
      else
        {
          nsh_output(vtbl, g_fmtarginvalid, argv[2]);
          return ERROR;
        }

      boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
    }
  else
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      return ERROR;
    }

  return 0;
}
#endif

/****************************************************************************
 * Name: cmd_poweroff
 ****************************************************************************/

#if defined(CONFIG_BOARDCTL_POWEROFF) && !defined(CONFIG_NSH_DISABLE_POWEROFF)
int cmd_poweroff(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  /* Invoke the BOARDIOC_POWEROFF board control to shutdown the board.  If
   * the board_power_off function returns, then it was not possible to power-
   * off the board due to some constraints.
   */

  if (argc > 1)
    {
      boardctl(BOARDIOC_POWEROFF, atoi(argv[1]));
    }
  else
    {
      boardctl(BOARDIOC_POWEROFF, EXIT_SUCCESS);
    }

  /* boarctl() will not return in any case.  It if does, it means that
   * there was a problem with the shutdown operation.
   */

  nsh_error(vtbl, g_fmtcmdfailed, argv[0], "boardctl", NSH_ERRNO);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_reboot
 ****************************************************************************/

#if defined(CONFIG_BOARDCTL_RESET) && !defined(CONFIG_NSH_DISABLE_REBOOT)
int cmd_reboot(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  /* Invoke the BOARDIOC_RESET board control to reset the board.  If
   * the board_reset() function returns, then it was not possible to
   * reset the board due to some constraints.
   */

  if (argc > 1)
    {
      boardctl(BOARDIOC_RESET, atoi(argv[1]));
    }
  else
    {
      boardctl(BOARDIOC_RESET, EXIT_SUCCESS);
    }

  /* boarctl() will not return in this case.  It if does, it means that
   * there was a problem with the reset operation.
   */

  nsh_error(vtbl, g_fmtcmdfailed, argv[0], "boardctl", NSH_ERRNO);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_rptun
 ****************************************************************************/

#if defined(CONFIG_RPTUN) && !defined(CONFIG_NSH_DISABLE_RPTUN)
int cmd_rptun(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  int fd;
  int cmd;

  if (argc < 3)
    {
      nsh_output(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

  if (strcmp(argv[1], "start") == 0)
    {
      cmd = RPTUNIOC_START;
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      cmd = RPTUNIOC_STOP;
    }
  else
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[1]);
      return ERROR;
    }

  fd = open(argv[2], 0);
  if (fd < 0)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[2]);
      return ERROR;
    }

  ioctl(fd, cmd, 0);

  close(fd);
  return 0;
}
#endif

/****************************************************************************
 * Name: cmd_uname
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_UNAME
int cmd_uname(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR const char *str;
  struct utsname info;
  unsigned int set;
  int option;
  bool badarg;
  bool first;
  int ret;
  int i;

  /* Get the uname options */

  set    = 0;
  badarg = false;

  while ((option = getopt(argc, argv, "asonrvmpi")) != ERROR)
    {
      switch (option)
        {
          case 'a':
            set = UNAME_ALL;
            break;

          case 'o':
          case 's':
            set |= UNAME_KERNEL;
            break;

#ifdef CONFIG_NET
          case 'n':
            set |= UNAME_NODE;
            break;
#endif

          case 'r':
            set |= UNAME_RELEASE;
            break;

          case 'v':
            set |= UNAME_VERISON;
            break;

          case 'm':
            set |= UNAME_MACHINE;
            break;

          case 'p':
            if (set != UNAME_ALL)
              {
                set |= UNAME_UNKNOWN;
              }
            break;

          case 'i':
            set |= UNAME_PLATFORM;
            break;

          case '?':
          default:
            nsh_error(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered, then return without processing the
   * command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* If nothing is provided on the command line, the default is -s */

  if (set == 0)
    {
      set = UNAME_KERNEL;
    }

  /* Get uname data */

  ret = uname(&info);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "uname", NSH_ERRNO);
      return ERROR;
    }

  /* Process each option */

  first = true;
  for (i = 0; set != 0; i++)
    {
      unsigned int mask = (1 << i);
      if ((set & mask) != 0)
        {
          set &= ~mask;
          switch (i)
            {
              case 0: /* print the kernel/operating system name */
                str = info.sysname;
                break;

#ifdef CONFIG_NET
              case 1: /* Print noname */
                str = info.nodename;
                break;
#endif
              case 2: /* Print the kernel release */
                str = info.release;
                break;

              case 3: /* Print the kernel version */
                str = info.version;
                break;

              case 4: /* Print the machine hardware name */
                str = info.machine;
                break;

              case 5: /* Print the machine platform name */
                str = BOARD_NAME;
                break;

              case 6: /* Print "unknown" */
                str = g_unknown;
                break;

              default:
                nsh_error(vtbl, g_fmtarginvalid, argv[0]);
                return ERROR;
            }

          if (!first)
            {
              nsh_output(vtbl, " ");
            }

          nsh_output(vtbl, str);
          first = false;
        }
    }

  nsh_output(vtbl, "\n");
  return OK;
}
#endif
