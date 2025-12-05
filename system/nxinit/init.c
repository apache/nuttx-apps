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
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/boardctl.h>
#include <sys/param.h>
#include <sys/wait.h>

#include "action.h"
#include "builtin.h"
#include "init.h"
#include "import.h"
#include "property.h"
#include "service.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MS2TIMESPEC(ts, ms) \
          ((ms) == INT_MAX ? NULL \
                           : ((ts)->tv_sec = (ms) / 1000, \
                              (ts)->tv_nsec = ((ms) % 1000) * 1000000, (ts)))

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
  int ret;
  int pid;

  /* prevent unused warning */

  UNUSED(name);
  UNUSED(status);

  for (; ; )
    {
      pid = waitpid(-1, &wstatus, WNOHANG);
      if (pid <= 0)
        {
          break;
        }
      else if (WIFEXITED(wstatus))
        {
          status = "status";
          ret = WEXITSTATUS(wstatus);
        }
      else if (WIFSIGNALED(wstatus))
        {
          status = "signal";
          ret = WTERMSIG(wstatus);
        }
      else
        {
          continue;
        }

      if (pid == am->pid_running)
        {
          name = am->running->argv[0];
          init_action_reap_command(am, ret);
        }

      service = init_service_find_by_pid(sm, pid);
      if (service != NULL)
        {
          name = service->argv[1];
          init_service_reap(service, ret);
        }

      init_log(service ? LOG_WARNING : LOG_DEBUG,
               "%s '%s' pid %d exited %s %d",
               service ? "Service" : "Command", name, pid, status, ret);
    }
}

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
static int init_boot_reason(FAR struct action_manager_s *am)
{
  static FAR const char *const resetcause[] =
    {
      [BOARDIOC_RESETCAUSE_NONE]        = "unkown",
      [BOARDIOC_RESETCAUSE_SYS_CHIPPOR] = "cold",
      [BOARDIOC_RESETCAUSE_SYS_RWDT]    = "watchdog",
      [BOARDIOC_RESETCAUSE_SYS_BOR]     = "undervoltage",
      [BOARDIOC_RESETCAUSE_CORE_DPSP]   = "warm",
      [BOARDIOC_RESETCAUSE_CORE_MWDT]   = "watchdog",
      [BOARDIOC_RESETCAUSE_CORE_RWDT]   = "watchdog",
      [BOARDIOC_RESETCAUSE_CPU_MWDT]    = "watchdog",
      [BOARDIOC_RESETCAUSE_CPU_RWDT]    = "watchdog",
      [BOARDIOC_RESETCAUSE_PIN]         = "powerkey",
      [BOARDIOC_RESETCAUSE_LOWPOWER]    = "lowpower",
      [BOARDIOC_RESETCAUSE_UNKOWN]      = "unkown",
    };

  static FAR const char * const resetflag[] =
    {
      [BOARDIOC_SOFTRESETCAUSE_USER_REBOOT]             = "reboot",
      [BOARDIOC_SOFTRESETCAUSE_PANIC]                   = "kernel_panic",
      [BOARDIOC_SOFTRESETCAUSE_ENTER_BOOTLOADER]        = "bootloader",
      [BOARDIOC_SOFTRESETCAUSE_ENTER_RECOVERY]          = "recovery",
      [BOARDIOC_SOFTRESETCAUSE_RESTORE_FACTORY]         = "factory_reset",
      [BOARDIOC_SOFTRESETCAUSE_RESTORE_FACTORY_INQUIRY] =
        "factory_reset_inquiry",
      [BOARDIOC_SOFTRESETCAUSE_THERMAL]                 = "thermal",
    };

  char buf[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];
  struct boardioc_reset_cause_s reset;
  FAR const char *value;
  int ret;

  memset(&reset, 0, sizeof(reset));
  ret = boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&reset);
  if (ret < 0)
    {
      init_err("boardctl BOARDIOC_RESET_CAUSE failed: %d", ret);
      return ret;
    }

  if (reset.cause >= nitems(resetcause))
    {
      reset.cause = nitems(resetcause) - 1;
    }

  if (reset.flag >= nitems(resetflag))
    {
      reset.flag = nitems(resetflag) - 1;
    }

  if (resetcause[reset.cause])
    {
      sprintf(buf, "%s,%" PRIu32 "", resetcause[reset.cause], reset.flag);
      value = buf;
    }
  else
    {
      value = resetflag[reset.flag];
    }

  return init_property_set(am->prop, "sys.boot.reason", value);
}
#else
#  define init_boot_reason(am) (0)
#endif

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

  struct init_poller_s poller[] =
    {
      {
        .init = init_property_init,
        .handle = init_property_handler,
        .deinit = init_property_deinit,
      },
    };

  struct pollfd pfds[nitems(poller)];
  sigset_t mask;
  size_t i;
  int r;

  sigfillset(&mask);
  r = sigprocmask(SIG_BLOCK, &mask, NULL);
  sigemptyset(&mask);
  if (r < 0)
    {
      init_err("sigprocmask failed %d", errno);
      return r;
    }

#ifdef CONFIG_USBDEV_TRACE
  usbtrace_enable(TRACE_BITSET);
#endif

  for (i = 0; i < nitems(poller); i++)
    {
      poller[i].sm = &sm;
      poller[i].am = &am;
      poller[i].pfd = &pfds[i];
      r = poller[i].init(&poller[i]);
      if (r < 0)
        {
          init_err("Init event %zu", i);
          goto out;
        }
    }

  r = init_parse_configs(parser);
  if (r < 0)
    {
      goto out;
    }

  init_dump_actions(&am.actions);
  init_dump_services(&sm.services);

  r = init_boot_reason(&am);
  if (r < 0)
    {
      goto out;
    }

  init_action_add_event(&am, "boot");

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

      r = ppoll(pfds, nitems(pfds), MS2TIMESPEC(&timeout, t), &mask);
      if (r < 0 && errno != EINTR)
        {
          init_err("Wait event");
          break;
        }

      for (i = 0; i < nitems(poller); i++)
        {
          if (poller[i].pfd->revents & poller[i].pfd->events)
            {
              poller[i].handle(&poller[i]);
            }
        }

      reap_process(&sm, &am);
    }

out:
  while (i--)
    {
      if (poller[i].deinit)
        {
          poller[i].deinit(&poller[i]);
        }
    }

  return r;
}
