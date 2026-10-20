/****************************************************************************
 * apps/system/nxinit/builtin.c
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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <netutils/netinit.h>

#ifdef CONFIG_RPTUN
#include <nuttx/rptun/rptun.h>
#endif

#include "builtin.h"
#include "init.h"
#include "property.h"
#include "service.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cmd_map_s
{
  FAR const char *cmd;
  uint8_t minargs;
  uint8_t maxargs;
  CODE int (*func)(FAR struct action_manager_s *, int, FAR char **);
};

#ifndef CONFIG_DISABLE_MOUNTPOINT
struct mount_flag_s
{
  FAR const char *name;
  unsigned long flag;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int cmd_trigger(FAR struct action_manager_s *am,
                       int argc, FAR char **argv);
static int cmd_exec_start(FAR struct action_manager_s *am,
                          int argc, FAR char **argv);
static int cmd_start(FAR struct action_manager_s *am,
                     int argc, FAR char **argv);
static int cmd_stop(FAR struct action_manager_s *am,
                    int argc, FAR char **argv);
#ifndef CONFIG_DISABLE_ENVIRON
static int cmd_set(FAR struct action_manager_s *am,
                   int argc, FAR char **argv);
#endif
#if CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX > 1
static int cmd_setprop(FAR struct action_manager_s *am,
                       int argc, FAR char **argv);
#endif
static int cmd_exec(FAR struct action_manager_s *am,
                    int argc, FAR char **argv);
static int cmd_class_start(FAR struct action_manager_s *am,
                           int argc, FAR char **argv);
static int cmd_class_stop(FAR struct action_manager_s *am,
                          int argc, FAR char **argv);
#ifdef CONFIG_BOARDCTL_BOOT_IMAGE
static int cmd_boot(FAR struct action_manager_s *am,
                    int argc, FAR char **argv);
#endif

#ifdef CONFIG_BOARDCTL_START_CPU
static int cmd_start_cpu(FAR struct action_manager_s *am,
                         int argc, FAR char **argv);
#endif
#ifdef CONFIG_NETUTILS_NETINIT
static int cmd_netinit(FAR struct action_manager_s *am,
                       int argc, FAR char **argv);
#endif
#ifdef CONFIG_BOARDCTL_FINALINIT
static int cmd_board_finalinit(FAR struct action_manager_s *am,
                               int argc, FAR char **argv);
#endif
#ifdef CONFIG_RPTUN
static int cmd_rptun(FAR struct action_manager_s *am,
                     int argc, FAR char **argv);
#endif
static int cmd_unlink(FAR struct action_manager_s *am,
                      int argc, FAR char **argv);
#ifndef CONFIG_DISABLE_MOUNTPOINT
static int cmd_mount(FAR struct action_manager_s *am,
                     int argc, FAR char **argv);
static int cmd_umount(FAR struct action_manager_s *am,
                      int argc, FAR char **argv);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
/* The flags of the "mount" command, as in the Android init language */

static const struct mount_flag_s g_mount_flags[] =
{
  {"defaults", 0},
  {"rw", 0},
  {"ro", MS_RDONLY},
  {"nosuid", MS_NOSUID},
  {"nodev", MS_NODEV},
  {"noexec", MS_NOEXEC},
  {"sync", MS_SYNCHRONOUS},
  {"remount", MS_REMOUNT},
  {"dirsync", MS_DIRSYNC},
  {"noatime", MS_NOATIME},
};
#endif

static const struct cmd_map_s g_builtin[] =
{
#ifdef CONFIG_BOARDCTL_FINALINIT
  {"board_finalinit", 1, 1, cmd_board_finalinit},
#endif
#ifdef CONFIG_BOARDCTL_BOOT_IMAGE
  {"boot", 1, 3, cmd_boot},
#endif
  {"class_start", 2, 2, cmd_class_start},
  {"class_stop", 2, 2, cmd_class_stop},
  {"exec", 3, 99, cmd_exec},
  {"exec_start", 2, 2, cmd_exec_start},
#ifndef CONFIG_DISABLE_MOUNTPOINT
  {"mount", 4, CONFIG_SYSTEM_NXINIT_ACTION_CMD_ARGS_MAX, cmd_mount},
#endif
#ifdef CONFIG_NETUTILS_NETINIT
  {"netinit", 1, 1, cmd_netinit},
#endif
#ifdef CONFIG_RPTUN
  {"rptun", 3, 3, cmd_rptun},
#endif
#ifndef CONFIG_DISABLE_ENVIRON
  {"set", 3, 3, cmd_set},
#endif
#if CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX > 1
  {"setprop", 3, 3, cmd_setprop},
#endif
  {"start", 2, 2, cmd_start},
#ifdef CONFIG_BOARDCTL_START_CPU
  {"start_cpu", 1, 3, cmd_start_cpu},
#endif
  {"stop", 2, 2, cmd_stop},
  {"trigger", 2, 2, cmd_trigger},
#ifndef CONFIG_DISABLE_MOUNTPOINT
  {"umount", 2, 2, cmd_umount},
#endif
  {"unlink", 2, 2, cmd_unlink},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_BOOT_IMAGE
static int cmd_boot(FAR struct action_manager_s *am,
                    int argc, FAR char **argv)
{
  struct boardioc_boot_info_s info;

  UNUSED(am);
  memset(&info, 0, sizeof(info));

  if (argc > 1)
    {
      info.path = argv[1];
    }

  if (argc > 2)
    {
      info.header_size = strtoul(argv[2], NULL, 0);
    }

  boardctl(BOARDIOC_BOOT_IMAGE, (uintptr_t)&info);
  init_err("boot image '%s' %" PRIu32 "", info.path ? info.path : "",
           info.header_size);
  return -ENOENT;
}
#endif

#ifdef CONFIG_BOARDCTL_START_CPU
static int cmd_start_cpu(FAR struct action_manager_s *am,
                         int argc, FAR char **argv)
{
  int cpuid = 0;

  if (argc > 1)
    {
      cpuid = strtol(argv[1], NULL, 0);
    }

  init_info("start cpu %d", cpuid);
  return boardctl(BOARDIOC_START_CPU, cpuid);
}
#endif

#ifdef CONFIG_NETUTILS_NETINIT
static int cmd_netinit(FAR struct action_manager_s *am,
                       int argc, FAR char **argv)
{
  UNUSED(am);
  return netinit_bringup();
}
#endif

#ifdef CONFIG_BOARDCTL_FINALINIT
static int cmd_board_finalinit(FAR struct action_manager_s *am,
                               int argc, FAR char **argv)
{
  UNUSED(am);
  return boardctl(BOARDIOC_FINALINIT, 0);
}
#endif

static int cmd_class_start(FAR struct action_manager_s *am,
                           int argc, FAR char **argv)
{
  return init_service_start_by_class(am->sm, argv[1]);
}

static int cmd_class_stop(FAR struct action_manager_s *am,
                          int argc, FAR char **argv)
{
  return init_service_stop_by_class(am->sm, argv[1]);
}

static int cmd_exec_start(FAR struct action_manager_s *am,
                          int argc, FAR char **argv)
{
  FAR struct service_s *service;

  service = init_service_find_by_name(am->sm, argv[1]);
  if (service == NULL)
    {
      init_err("No such service '%s'", argv[1]);
      return -EINVAL;
    }

  return init_service_start(service);
}

static int cmd_start(FAR struct action_manager_s *am,
                     int argc, FAR char **argv)
{
  int ret;

  ret = cmd_exec_start(am, argc, argv);
  return ret < 0 ? ret : 0;
}

static int cmd_stop(FAR struct action_manager_s *am,
                    int argc, FAR char **argv)
{
  FAR struct service_s *service;

  service = init_service_find_by_name(am->sm, argv[1]);
  if (service == NULL)
    {
      init_err("No such service '%s'", argv[1]);
      return -EINVAL;
    }

  return init_service_stop(service);
}

#ifndef CONFIG_DISABLE_ENVIRON
static int cmd_set(FAR struct action_manager_s *am,
                   int argc, FAR char **argv)
{
  UNUSED(am);
  init_info("setenv '%s' '%s'", argv[1], argv[2]);
  return setenv(argv[1], argv[2], 1);
}
#endif

#if CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX > 1
static int cmd_setprop(FAR struct action_manager_s *am, int argc,
                       FAR char **argv)
{
  return init_property_set(am->prop, argv[1], argv[2]);
}
#endif

static int cmd_trigger(FAR struct action_manager_s *am,
                       int argc, FAR char **argv)
{
  return init_action_add_event(am, argv[1]);
}

static int cmd_exec(FAR struct action_manager_s *am,
                    int argc, FAR char **argv)
{
  int i;

  for (i = 0; i < argc; i++)
    {
      if (!strcmp(argv[i], "--"))
        {
          i++;
          return init_builtin_run(am, argc - i, &argv[i]);
        }
    }

  return -EINVAL;
}

#ifdef CONFIG_RPTUN
static int cmd_rptun(FAR struct action_manager_s *am,
                     int argc, FAR char **argv)
{
  int fd;
  int ret;

  UNUSED(am);

  fd = open(argv[2], O_WRONLY | O_CLOEXEC);
  if (fd < 0)
    {
      init_err("rptun %s: open '%s' failed: %d",
               argv[1], argv[2], errno);
      return -errno;
    }

  if (!strcmp(argv[1], "start"))
    {
      ret = ioctl(fd, RPTUNIOC_START, 0);
    }
  else if (!strcmp(argv[1], "stop"))
    {
      ret = ioctl(fd, RPTUNIOC_STOP, 0);
    }
  else
    {
      init_err("rptun: unknown command '%s'", argv[1]);
      ret = -EINVAL;
    }

  if (ret < 0)
    {
      init_err("rptun %s: failed: %d", argv[1], errno);
    }

  close(fd);
  return ret < 0 ? ret : 0;
}
#endif

static int cmd_unlink(FAR struct action_manager_s *am,
                      int argc, FAR char **argv)
{
  int ret;

  UNUSED(am);

  ret = unlink(argv[1]);
  if (ret < 0)
    {
      init_err("unlink '%s' failed: %d", argv[1], errno);
    }

  return ret;
}

#ifndef CONFIG_DISABLE_MOUNTPOINT
/* Copy the comma-separated list in options to data without the flags
 * (g_mount_flags), and return the flags.  options does not change, as the
 * command can run again.  data has CONFIG_SYSTEM_NXINIT_RC_LINE_MAX bytes,
 * and an argument is part of one line.
 */

static unsigned long mount_parse_options(FAR const char *options,
                                         FAR char *data)
{
  FAR const char *end;
  FAR char *dst = data;
  unsigned long flags = 0;
  size_t len;
  size_t j;

  while (*options != '\0')
    {
      end = strchrnul(options, ',');
      len = end - options;

      for (j = 0; j < nitems(g_mount_flags); j++)
        {
          if (!strncmp(g_mount_flags[j].name, options, len) &&
              g_mount_flags[j].name[len] == '\0')
            {
              flags |= g_mount_flags[j].flag;
              break;
            }
        }

      if (j == nitems(g_mount_flags) && len > 0)
        {
          if (dst != data)
            {
              *dst++ = ',';
            }

          memcpy(dst, options, len);
          dst += len;
        }

      options = *end == ',' ? end + 1 : end;
    }

  *dst = '\0';
  return flags;
}

/* mount <type> <device> <dir> [<flag>]* [<options>]
 * mount -t <type> [-o <options>] [<device>] <dir>
 *
 * <flag> is one of g_mount_flags.  <options> is a comma-separated list of
 * flags and file system options (the data of mount()), as in the NSH
 * command.  The second form is the one of the NSH command.
 */

static int cmd_mount(FAR struct action_manager_s *am,
                     int argc, FAR char **argv)
{
  char data[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];
  FAR const char *options = NULL;
  FAR const char *source = NULL;
  FAR const char *target;
  FAR const char *type = NULL;
  unsigned long flags = 0;
  int ret;
  int i;

  UNUSED(am);

  data[0] = '\0';
  if (argc > 1 && argv[1][0] == '-')
    {
      for (i = 1; i < argc - 1 && argv[i][0] == '-'; i += 2)
        {
          if (!strcmp(argv[i], "-t"))
            {
              type = argv[i + 1];
            }
          else if (!strcmp(argv[i], "-o"))
            {
              options = argv[i + 1];
            }
          else
            {
              break;
            }
        }

      if (type == NULL || i >= argc || argv[i][0] == '-' || argc - i > 2)
        {
          init_err("mount: bad arguments");
          return -EINVAL;
        }

      if (argc - i == 2)
        {
          source = argv[i++];
        }

      target = argv[i];
      if (options != NULL)
        {
          flags = mount_parse_options(options, data);
        }
    }
  else
    {
      type   = argv[1];
      source = argv[2];
      target = argv[3];

      for (i = 4; i < argc; i++)
        {
          flags |= mount_parse_options(argv[i], data);
          if (data[0] != '\0' && i != argc - 1)
            {
              init_err("mount '%s': unknown flag '%s'", target, argv[i]);
              return -EINVAL;
            }
        }
    }

  if (mount(source, target, type, flags,
            data[0] != '\0' ? data : NULL) < 0)
    {
      ret = -errno;
      init_err("mount %s '%s' on '%s' failed: %d",
               type, source ? source : "", target, -ret);
      return ret;
    }

  return 0;
}

/* umount <dir> */

static int cmd_umount(FAR struct action_manager_s *am,
                      int argc, FAR char **argv)
{
  int ret;

  UNUSED(am);

  if (umount(argv[1]) < 0)
    {
      ret = errno;
      init_err("umount '%s' failed: %d", argv[1], ret);
      return -ret;
    }

  return 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_builtin_run
 *
 * Description:
 *   Execute the builtin command of NxInit, builtin App, or NSH builtin.
 *
 * Input Parameters:
 *   am   - Instance of Action Manager.
 *   argc - Argument count.
 *   argv - A pointer to an array of string arguments. The end of the array
 *          is indicated with a NULL entry.
 *
 * Returned Value:
 *   Returns 0 (for NxInit builtin) or a PID (for builtin App or NSH builtin)
 *   on success; Returns the negative value of errno on failure.
 ****************************************************************************/

int init_builtin_run(FAR struct action_manager_s *am,
                     int argc, FAR char **argv)
{
  char cmd[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];
  posix_spawnattr_t attr;
  FAR char *args[4];
  sigset_t blocked;
  sigset_t builtin;
  sigset_t mask;
  pid_t pid;
  size_t i;
  int ret;

  for (i = 0; i < nitems(g_builtin); i++)
    {
      if (!strcmp(g_builtin[i].cmd, argv[0]))
        {
          if (argc < g_builtin[i].minargs || argc > g_builtin[i].maxargs)
            {
              init_err("Executing command '%s': invalid argument", argv[0]);
              init_dump_args(argc, argv);
              return -EINVAL;
            }

          init_info("Executing command '%s'", argv[0]);

          /* Init blocks all signals.  Run the builtin command with the
           * original mask, as tasks and threads it creates (e.g. the NTP
           * client and the network monitor started by netinit) inherit
           * the mask of this task.  Keep SIGCHLD blocked so that a child
           * exiting meanwhile still wakes up ppoll() in init.
           */

          builtin = am->sigmask;
          sigaddset(&builtin, SIGCHLD);
          sigprocmask(SIG_SETMASK, &builtin, &blocked);
          ret = g_builtin[i].func(am, argc, argv);
          sigprocmask(SIG_SETMASK, &blocked, NULL);
          return ret;
        }
    }

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      init_err("posix_spawnattr_init %d", ret);
      return -ret;
    }

  ret = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK);
  if (ret != 0)
    {
      init_err("posix_spawnattr_setflags %d", ret);
      posix_spawnattr_destroy(&attr);
      return -ret;
    }

  sigemptyset(&mask);
  ret = posix_spawnattr_setsigmask(&attr, &mask);
  if (ret != 0)
    {
      init_err("posix_spawnattr_setsigmask %d", ret);
      posix_spawnattr_destroy(&attr);
      return -ret;
    }

  for (i = 0, cmd[i] = '\0'; i < argc; i++)
    {
      strlcat(cmd, argv[i], sizeof(cmd));
      if (i < argc - 1)
        {
          strcat(cmd, " ");
        }
    }

  for (; ; )
    {
      ret = posix_spawnp(&pid, argv[0], NULL, &attr, argv, NULL);
      if (ret == 0)
        {
          init_debug("executed command '%s' pid %d", cmd, pid);
          break;
        }

      if (!strcmp("sh", argv[0]))
        {
          init_err("executing command '%s': %d", cmd, ret);
          init_dump_args(argc, argv);
          pid = -ret;
          break;
        }

      init_debug("command '%s': %d", argv[0], ret);
      args[0] = "sh";
      args[1] = "-c";
      args[2] = cmd;
      args[3] = NULL;
      argc = nitems(args) - 1;
      argv = args;
    }

  posix_spawnattr_destroy(&attr);
  return pid;
}
