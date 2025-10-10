/****************************************************************************
 * apps/system/nxinit/service.c
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

#include <nuttx/clock.h>
#include <sys/boardctl.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <spawn.h>
#include <sys/param.h>

#include "init.h"
#include "parser.h"
#include "service.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SYSTEM_NXINIT_SERVICE_GENTLE_KILL_TIMEOUT 200

#define check_flags(s, f) ((s)->flags & (f))

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
#  define dump_flags(fmt, flags) \
          do \
            { \
              size_t _i; \
              for (_i = 0; _i < nitems(g_flag_str); _i++) \
                { \
                  if (g_flag_str[_i].flag & (flags)) \
                    { \
                      init_debug(fmt, g_flag_str[_i].str); \
                    } \
                } \
            } \
          while(0)
#else
#  define dump_flags(...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cmd_map_s
{
  FAR const char *cmd;
  uint8_t minargs;
  uint8_t maxargs;
  int (*func)(FAR struct service_manager_s *, int, FAR char **);
};

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
struct flag_str_s
{
  uint32_t flag;
  FAR const char *str;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int option_class(FAR struct service_manager_s *sm,
                        int argc, FAR char **argv);
static int option_gentle_kill(FAR struct service_manager_s *sm,
                              int argc, FAR char **argv);
static int option_restart_period(FAR struct service_manager_s *sm,
                                 int argc, FAR char **argv);
static int option_override(FAR struct service_manager_s *sm,
                           int argc, FAR char **argv);
static int option_oneshot(FAR struct service_manager_s *sm,
                          int argc, FAR char **argv);
#ifdef CONFIG_BOARDCTL_RESET
static int option_reboot_on_failure(FAR struct service_manager_s *sm,
                                    int argc, FAR char **argv);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct cmd_map_s g_option[] =
{
  {"class", 2, 99, option_class},
  {"gentle_kill", 1, 1, option_gentle_kill},
  {"restart_period", 2, 2, option_restart_period},
  {"override", 1, 1, option_override},
  {"oneshot", 1, 1, option_oneshot},
#ifdef CONFIG_BOARDCTL_RESET
  {"reboot_on_failure", 2, 2, option_reboot_on_failure},
#endif
};

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
static const struct flag_str_s g_flag_str[] =
{
  {SVC_DISABLED, "disabled"},
  {SVC_ONESHOT, "oneshot"},
  {SVC_RUNNING, "running"},
  {SVC_RESTARTING, "restarting"},
  {SVC_GENTLE_KILL, "gentle_kill"},
  {SVC_REMOVE, "remove"},
  {SVC_SIGKILL, "sigkill"},
  {SVC_OVERRIDE, "override"},
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void add_flags(FAR struct service_s *service, uint32_t flags)
{
  init_debug("Service '%s' flag 0x%" PRIx32 " add 0x%" PRIx32 "",
             service->argv[1], service->flags, flags);
  dump_flags("  +flag '%s'", flags);

  service->flags |= flags;
}

static void remove_flags(FAR struct service_s *service, uint32_t flags)
{
  init_debug("Service '%s' flag 0x%" PRIx32 " add 0x%" PRIx32 "",
             service->argv[1], service->flags, flags);
  dump_flags("  -flag '%s'", flags);

  service->flags &= ~flags;
}

static int kill_service(FAR struct service_s *service, int signo)
{
  int ret;

  ret = kill(service->pid, signo);
  if (ret < 0)
    {
      ret = -errno;
      if (ret == -ESRCH)
        {
          init_service_reap(service, 0);
        }
    }

  init_log(ret < 0 ? LOG_ERR : LOG_WARNING,
           "sent signal %d to service '%s' pid %d %d",
           signo, service->argv[1], service->pid, ret);

  return ret;
}

static void remove_service(FAR struct service_s *service)
{
  FAR struct service_class_s *class;
  FAR struct service_class_s *tmp;
  int i;

  init_warn("Removing service '%s' ...", service->argv[1]);
  list_for_every_entry_safe(&service->classes, class, tmp,
                            struct service_class_s, node)
    {
      list_delete(&class->node);
      free(class);
    }

  for (i = 0; i < service->argc; i++)
    {
      free(service->argv[i]);
    }

  list_delete(&service->node);
  free(service);
}

static int option_class(FAR struct service_manager_s *sm,
                        int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);
  size_t len = strlen(argv[1]) + 1;
  FAR struct service_class_s *c;

  list_for_every_entry(&s->classes, c, struct service_class_s, node)
    {
      if (!strcmp(c->name, argv[1]))
        {
          return 0;
        }
    }

  c = malloc(sizeof(*c) + len);
  if (c == NULL)
    {
      init_err("Alloc class");
      return -errno;
    }

  memcpy(c->name, argv[1], len);
  list_add_tail(&s->classes, &c->node);
  return 0;
}

static int option_gentle_kill(FAR struct service_manager_s *sm,
                              int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);

  add_flags(s, SVC_GENTLE_KILL);
  return 0;
}

static int option_restart_period(FAR struct service_manager_s *sm,
                                 int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);

  s->restart_period = atoi(argv[1]);
  return 0;
}

static int option_override(FAR struct service_manager_s *sm,
                           int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);

  add_flags(s, SVC_OVERRIDE);
  return 0;
}

static int option_oneshot(FAR struct service_manager_s *sm,
                          int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);

  add_flags(s, SVC_ONESHOT);
  return 0;
}

#ifdef CONFIG_BOARDCTL_RESET
static int option_reboot_on_failure(FAR struct service_manager_s *sm,
                                    int argc, FAR char **argv)
{
  FAR struct service_s *s = list_last_entry(&sm->services, struct service_s,
                                            node);
  s->reset_reason = atoi(argv[1]);
  return 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_service_refresh
 *
 * Description:
 *   Check if any services need to be restarted, force terminate(SIGKILL), or
 *   deleted.
 *
 * Input Parameters:
 *   sm - Instance of Service Manager
 *
 * Returned Value:
 *   The expected time interval until the next call is returned, INT_MAX for
 *   block indefinitely;
 ****************************************************************************/

int init_service_refresh(FAR struct service_manager_s *sm)
{
  FAR struct service_s *service;
  FAR struct service_s *tmp;
  struct timespec diff;
  struct timespec cur;
  int min = INT_MAX;
  int ms;

  clock_gettime(CLOCK_MONOTONIC, &cur);
  list_for_every_entry_safe(&sm->services, service, tmp, struct service_s,
                            node)
    {
      if (check_flags(service, SVC_RESTARTING))
        {
          clock_timespec_subtract(&cur, &service->time_started, &diff);
          ms = TIMESPEC2MS(diff);
          if (ms >= service->restart_period)
            {
              init_service_start(service);
              continue;
            }

          min = MIN(min, service->restart_period - ms);
        }
      else if (check_flags(service, SVC_RUNNING) &&
               check_flags(service, SVC_GENTLE_KILL) &&
               check_flags(service, SVC_DISABLED))
        {
          if (check_flags(service, SVC_SIGKILL))
            {
              continue;
            }

          clock_timespec_subtract(&cur, &service->time_kill, &diff);
          ms = TIMESPEC2MS(diff);
          if (ms >= SYSTEM_NXINIT_SERVICE_GENTLE_KILL_TIMEOUT)
            {
              init_warn("Service '%s' pid %d timeout for gentle kill",
                        service->argv[1], service->pid);
              kill_service(service, SIGKILL);
              add_flags(service, SVC_SIGKILL);
              continue;
            }

          min = MIN(min, SYSTEM_NXINIT_SERVICE_GENTLE_KILL_TIMEOUT - ms);
        }
      else if (check_flags(service, SVC_REMOVE) &&
               check_flags(service, SVC_DISABLED) &&
               !check_flags(service, SVC_RUNNING))
        {
          remove_service(service);
        }
    }

  return min;
}

FAR struct service_s *
init_service_find_by_name(FAR struct service_manager_s *sm,
                          FAR const char *name)
{
  FAR struct service_s *service;

  list_for_every_entry(&sm->services, service, struct service_s, node)
    {
      if (!strcmp(name, service->argv[1]))
        {
          return service;
        }
    }

  return NULL;
}

FAR struct service_s *
init_service_find_by_pid(FAR struct service_manager_s *sm, const int pid)
{
  FAR struct service_s *service;

  list_for_every_entry(&sm->services, service, struct service_s, node)
    {
      if (pid == service->pid)
        {
          return service;
        }
    }

  return NULL;
}

void init_service_reap(FAR struct service_s *service, int status)
{
#ifdef CONFIG_BOARDCTL_RESET
  int ret;

  if (status && service->reset_reason >= 0)
    {
      init_err("Reboot on failure of service '%s' reason %d",
               service->argv[1], service->reset_reason);
      ret = boardctl(BOARDIOC_RESET, service->reset_reason);
      if (ret < 0)
        {
          init_err("Reset failed %d", errno);
        }
    }
#else
  UNUSED(status);
#endif

  remove_flags(service, SVC_RUNNING);
  if (check_flags(service, SVC_ONESHOT))
    {
      add_flags(service, SVC_DISABLED | SVC_REMOVE);
    }

  if (!check_flags(service, SVC_DISABLED))
    {
      add_flags(service, SVC_RESTARTING);
    }
}

int init_service_start(FAR struct service_s *service)
{
  int ret;
  int pid;

  init_debug("Starting service '%s' ...", service->argv[1]);

  if (check_flags(service, SVC_RUNNING))
    {
      init_info("Service '%s' already started pid %d", service->argv[1],
                service->pid);
      return service->pid;
    }

  ret = posix_spawnp(&pid, service->argv[2], NULL, NULL, &service->argv[2],
                     environ);
  if (ret != 0)
    {
      init_err("Starting service '%s': %d", service->argv[1], ret);
      init_service_reap(service, ret);
      return -ret;
    }

  service->pid = pid;
  clock_gettime(CLOCK_MONOTONIC, &service->time_started);
  add_flags(service, SVC_RUNNING);
  remove_flags(service, SVC_RESTARTING);
  remove_flags(service, SVC_DISABLED);
  init_info("Started service '%s' pid %d", service->argv[1], service->pid);

  return service->pid;
}

int init_service_stop(FAR struct service_s *service)
{
  init_info("Stopping service '%s' ...", service->argv[1]);

  if (check_flags(service, SVC_RUNNING | SVC_RESTARTING))
    {
      if (check_flags(service, SVC_DISABLED))
        {
          init_warn("Service '%s' is exiting", service->argv[1]);
          return -EINPROGRESS;
        }
    }
  else
    {
      init_warn("Service '%s' is not running", service->argv[1]);
      return -ESRCH;
    }

  add_flags(service, SVC_DISABLED);
  if (check_flags(service, SVC_RESTARTING))
    {
      remove_flags(service, SVC_RESTARTING);
    }

  if (check_flags(service, SVC_GENTLE_KILL))
    {
      init_debug("Gentle kill");
      remove_flags(service, SVC_SIGKILL);
      kill_service(service, SIGTERM);
      clock_gettime(CLOCK_MONOTONIC, &service->time_kill);
      return 0;
    }

  return kill_service(service, SIGKILL);
}

int init_service_start_by_class(FAR struct service_manager_s *sm,
                                FAR const char *name)
{
  FAR struct service_class_s *class;
  FAR struct service_s *service;
  int ret;

  list_for_every_entry(&sm->services, service, struct service_s, node)
    {
      list_for_every_entry(&service->classes, class, struct service_class_s,
                           node)
        {
          if (!strcmp(name, class->name))
            {
              ret = init_service_start(service);
              if (ret < 0)
                {
                  return ret;
                }

              break;
            }
        }
    }

  return 0;
}

int init_service_stop_by_class(FAR struct service_manager_s *sm,
                               FAR const char *name)
{
  FAR struct service_class_s *class;
  FAR struct service_s *service;
  int ret;

  list_for_every_entry(&sm->services, service, struct service_s, node)
    {
      list_for_every_entry(&service->classes, class, struct service_class_s,
                           node)
        {
          if (!strcmp(name, class->name))
            {
              ret = init_service_stop(service);
              if (ret < 0)
                {
                  return ret;
                }

              break;
            }
        }
    }

  return 0;
}

int init_service_parse(FAR const struct parser_s *parser,
                       bool create, FAR char *buf)
{
  FAR char *argv[CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX];
  FAR struct service_manager_s *sm = parser->priv;
  FAR struct service_s *s;
  int argc;
  size_t i;

  if (create)
    {
      s = calloc(1, sizeof(*s));
      if (s == NULL)
        {
          init_err("Alloc service");
          return -errno;
        }

      s->argc = init_parse_arguments(buf, true, nitems(s->argv), s->argv);
      if (s->argc < 3)
        {
          init_err("Get arguments %d", s->argc);
          free(s);
          return -EINVAL;
        }

      s->restart_period = CONFIG_SYSTEM_NXINIT_SERVICE_RESTART_PERIOD;
#ifdef CONFIG_BOARDCTL_RESET
      s->reset_reason = -1;
#endif
      list_initialize(&s->classes);
      list_add_tail(&sm->services, &s->node);
    }
  else
    {
      argc = init_parse_arguments(buf, false, nitems(argv), argv);
      for (i = 0; i < nitems(g_option); i++)
        {
          if (!strcmp(g_option[i].cmd, argv[0]))
            {
              init_info("Apply option[%zu] %p for %s", i, g_option[i].func,
                        argv[0]);
              if (argc < g_option[i].minargs || argc > g_option[i].maxargs)
                {
                  break;
                }

              init_dump_args(argc, argv);
              return g_option[i].func(sm, argc, argv);
            }
        }

      init_err("'%s': unknown option / invalid argument", argv[0]);
      return -EINVAL;
    }

  return 0;
}

int init_service_check(FAR const struct parser_s *parser)
{
  FAR struct service_manager_s *sm = parser->priv;
  FAR struct service_s *tmp;
  FAR struct service_s *s;

  list_for_every_entry(&sm->services, s, struct service_s, node)
    {
      tmp = list_next_entry(s, struct service_s, node);
      list_for_every_entry_from(&sm->services, tmp, struct service_s, node)
        {
          if (!strcmp(s->argv[1], tmp->argv[1]))
            {
              if (!check_flags(tmp, SVC_OVERRIDE))
                {
                  init_err("Redefined service '%s'", tmp->argv[1]);
                  init_dump_service(s);
                  init_dump_service(tmp);
                  return -EEXIST;
                }

              init_info("Remove duplicate definition of service '%s'",
                        tmp->argv[1]);
              add_flags(s, SVC_DISABLED | SVC_REMOVE);
            }
        }
    }

  return 0;
}

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
void init_dump_service(FAR struct service_s *s)
{
  FAR struct service_class_s *c;
  int i;

  init_debug("Service %p name '%s' path '%s'", s, s->argv[1], s->argv[2]);

  init_debug("  pid: %d", s->pid);
  init_debug("  arguments:");
  for (i = 0; s->argv[i] && i < nitems(s->argv); i++)
    {
      init_debug("      [%d] '%s'", i, s->argv[i]);
    }

  init_debug("  classes:");
  list_for_every_entry(&s->classes, c, struct service_class_s, node)
    {
      init_debug("    '%s'", c->name);
    }

  init_debug("  restart_period: %d", s->restart_period);
#ifdef CONFIG_BOARDCTL_RESET
  init_debug("  reboot_on_failure: %d", s->reset_reason);
#endif

  if (s->flags)
    {
      init_debug("  flags:");
      dump_flags("    '%s'", s->flags);
    }
}

void init_dump_services(FAR struct list_node *head)
{
  FAR struct service_s *service;

  init_debug("== Dump Services ==");
  list_for_every_entry(head, service, struct service_s, node)
    {
      init_dump_service(service);
    }
}
#endif
