/****************************************************************************
 * apps/system/nxinit/init.c
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

#include <errno.h>
#include <poll.h>
#include <sys/param.h>
#include <sys/boardctl.h>
#include <sys/wait.h>

#include <netutils/netinit.h>

#include "action.h"
#include "builtin.h"
#include "init.h"
#include "import.h"
#include "service.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MS2TIMESPEC(ts, ms) \
          ((ms) == INT_MAX ? NULL \
                           : ((ts)->tv_sec = (ms) / 1000, \
                              (ts)->tv_nsec = ((ms) % 1000) * 1000000, (ts)))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct init_event_s
{
  FAR struct pollfd *pfd;
  FAR void *priv;
  FAR struct service_manager_s *sm;
  FAR struct action_manager_s *am;
  CODE int  (*init)   (FAR struct init_event_s *);
  CODE void (*handle) (FAR struct init_event_s *);
  CODE void (*deinit) (FAR struct init_event_s *);
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void reap_process(FAR struct service_manager_s *sm,
                         FAR struct action_manager_s *am)
{
  FAR const char *name = "unknown";
  FAR const char *status;
  FAR struct service_s *service;
  int wstatus;
  int pid;

  for (; ; )
    {
      pid = waitpid(-1,
#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
                    &wstatus,
#else
                    NULL,
#endif
                    WNOHANG);
      if (pid <= 0)
        {
          break;
        }
      else if (WIFEXITED(wstatus))
        {
          status = "status";
          wstatus = WEXITSTATUS(wstatus);
        }
      else if (WIFSIGNALED(wtatus))
        {
          status = "signal";
          wstatus = WTERMSIG(wstatus);
        }
      else
        {
          continue;
        }

      service = init_service_find_by_pid(sm, pid);
      if (service != NULL)
        {
          name = service->argv[1];
          init_service_reap(service);
        }
      else if (pid == am->pid_running)
        {
          name = am->running->argv[0];
          init_action_reap_command(am);
        }

      init_log(service ? LOG_WARNING : LOG_DEBUG,
               "%s '%s' pid %d exited %s %d",
               service ? "Service" : "Command", name, pid, status, wstatus);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct service_manager_s sm =
    {
      .services = LIST_INITIAL_VALUE(sm.services),
    };

  struct action_manager_s am =
    {
      .actions = LIST_INITIAL_VALUE(am.actions),
      .ready_actions = LIST_INITIAL_VALUE(am.ready_actions),
      .events = { 0 },
      .current = NULL,
      .running = NULL,
      .pid_running = -1,
      .sm = &sm,
    };

  const struct parser_s parser[] =
    {
      {"import", init_import_parse, NULL, NULL},
      {"on", init_action_parse, NULL, &am},
      {"service", init_service_parse, init_service_check, &sm},
      {NULL},
    };

  struct init_event_s ev[] =
    {
    };

  struct pollfd pfds[nitems(ev)];
  size_t i;
  int r;

#ifdef CONFIG_USBDEV_TRACE
  usbtrace_enable(TRACE_BITSET);
#endif

  for (i = 0; i < nitems(ev); i++)
    {
      ev[i].sm = &sm;
      ev[i].am = &am;
      ev[i].pfd = &pfds[i];
      r = ev[i].init(&ev[i]);
      if (r < 0)
        {
          init_err("Init event %zu", i);
          goto out;
        }
    }

  r = init_parse_config_file(parser, "/etc/init.d/init.rc");
  if (r < 0)
    {
      goto out;
    }

  init_dump_actions(&am.actions);
  init_dump_services(&sm.services);

  init_action_add_event(&am, "boot");

  boardctl(BOARDIOC_INIT, 0);
  init_action_add_event(&am, "init");

#ifdef CONFIG_NETUTILS_NETINIT
  netinit_bringup();
  init_action_add_event(&am, "netinit");
#endif

#ifdef CONFIG_BOARDCTL_FINALINIT
  boardctl(BOARDIOC_FINALINIT, 0);
  init_action_add_event(&am, "finalinit");
#endif

  for (; ; )
    {
      int t1 = init_service_refresh(&sm);
      int t2 = init_action_run_command(&am);
      struct timespec timeout;
      int t = MIN(t1, t2);

      if (t1 < 0)
        {
          r = t1;
          break;
        }

      if (t2 < 0)
        {
          r = t2;
          break;
        }

      r = ppoll(pfds, nitems(pfds), MS2TIMESPEC(&timeout, t), NULL);
      if (r < 0 && errno != EINTR)
        {
          init_err("Wait event");
          break;
        }

      for (i = 0; i < nitems(ev); i++)
        {
          if (ev[i].pfd->revents & ev[i].pfd->events)
            {
              ev[i].handle(&ev[i]);
            }
        }

      reap_process(&sm, &am);
    }

out:
  while (i--)
    {
      if (ev[i].deinit)
        {
          ev[i].deinit(&ev[i]);
        }
    }

  return r;
}
