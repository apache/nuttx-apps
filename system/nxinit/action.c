/****************************************************************************
 * apps/system/nxinit/action.c
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>

#include "action.h"
#include "builtin.h"
#include "init.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum number of parameters for the action trigger.
 * Format: `on <trigger>`
 */

#define ACTION_ARGUMENTS_MAX 2

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
static void init_dump_action(FAR struct action_s *action);
#else
#  define init_dump_action(a)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
static void init_dump_action(FAR struct action_s *action)
{
  FAR struct action_cmd_s *cmd;

  init_debug("Action %p", action);
  init_debug("  event trigger: '%s'", action->event ? action->event : "");
  list_for_every_entry(&action->cmds, cmd, struct action_cmd_s, node)
    {
      init_dump_args(cmd->argc, cmd->argv);
    }
}
#endif

static void add_ready(FAR struct action_manager_s *am,
                      FAR struct action_s *a)
{
  FAR struct action_s *ready;
#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
  FAR struct action_cmd_s *cmd;

  list_for_every_entry(&a->cmds, cmd, struct action_cmd_s, node)
    {
      init_debug("Add ready %p %s", cmd, cmd->argv[0]);
    }
#endif

  list_for_every_entry(&am->ready_actions, ready, struct action_s,
                       ready_node)
    {
      if (ready == a)
        {
          init_debug("Action %p(%s) already on the queue", a,
                     a->event ? a->event : "");
          init_dump_action(a);
          return;
        }
    }

  list_add_tail(&am->ready_actions, &a->ready_node);
}

static void update_ready(FAR struct action_manager_s *am)
{
  size_t i;

  /* Actions with event trigger */

  for (i = 0; i < nitems(am->events); i++)
    {
      if (!am->events[i])
        {
          continue;
        }

      am->current = list_prepare_entry(am->current, &am->actions,
                                       struct action_s, node);
      list_for_every_entry_continue(am->current, &am->actions,
                                    struct action_s, node)
        {
          if (am->current->event && !strcmp(am->current->event,
                                            am->events[i]))
            {
              add_ready(am, am->current);
              return;
            }
        }

      init_debug("Remove event [%zu] '%s'", i, am->events[i]);
      am->current = NULL;
      free(am->events[i]);
      am->events[i] = NULL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_action_add_event(FAR struct action_manager_s *am,
                          FAR const char *event)
{
  int ret = -ENOBUFS;
  size_t i;

  for (i = 0; i < nitems(am->events); i++)
    {
      if (am->events[i])
        {
          continue;
        }

      am->events[i] = strdup(event);
      if (am->events[i])
        {
          init_debug("Add event [%zu] '%s'", i, am->events[i]);
          return 0;
        }

      ret = -errno;
      break;
    }

  init_warn("Drop event '%s' %d", event, ret);
  return ret;
}

/****************************************************************************
 * Name: init_action_run_command
 *
 * Description:
 *   Execute the ready commands in the action.
 *
 * Input Parameters:
 *   am - Instance of Action Manager
 *
 * Returned Value:
 *   The expected next call interval (in milliseconds), where INT_MAX
 *   indicates blocking.
 ****************************************************************************/

int init_action_run_command(FAR struct action_manager_s *am)
{
  FAR struct action_s *ready;
  int ret;

  if (am->pid_running != -1)
    {
      init_debug("Waiting '%s' pid %d", am->running->argv[0],
                 am->pid_running);
      return INT_MAX;
    }

  update_ready(am);
  if (list_is_empty(&am->ready_actions))
    {
      return INT_MAX;
    }

  ready = list_peek_head_type(&am->ready_actions, struct action_s,
                              ready_node);

  if (!am->running)
    {
      am->running = list_peek_head_type(&ready->cmds,
                                        struct action_cmd_s, node);
    }

  ret = init_builtin_run(am, am->running->argc, am->running->argv);
  if (ret > 0)
    {
      am->pid_running = ret;
    }
  else
    {
      init_action_reap_command(am);
    }

  return 0;
}

void init_action_reap_command(FAR struct action_manager_s *am)
{
  FAR struct action_s *ready = list_peek_head_type(&am->ready_actions,
                                                   struct action_s,
                                                   ready_node);

  am->pid_running = -1;
  if (list_is_tail(&ready->cmds, &am->running->node))
    {
      am->running = NULL;
      list_delete(&ready->ready_node);
    }
  else
    {
      am->running = list_next_entry(am->running, struct action_cmd_s, node);
    }
}

int init_action_parse(FAR const struct parser_s *parser,
                      bool create, FAR char *buf)
{
  FAR struct action_manager_s *am = parser->priv;
  FAR char *argv[ACTION_ARGUMENTS_MAX];
  FAR struct action_cmd_s *cmd;
  FAR struct action_s *a;
  int ret;

  if (create)
    {
      ret = init_parse_arguments(buf, false, nitems(argv), argv);
      if (ret < 2)
        {
          init_err("Invalid argument: %s", buf);
          return -EINVAL;
        }

      a = calloc(1, sizeof(*a));
      if (a == NULL)
        {
          init_err("Alloc action");
          return -errno;
        }

      list_initialize(&a->cmds);

      a->event = strdup(argv[--ret]);
      if (a->event == NULL)
        {
          init_err("Event trigger");
          free(a);
          return -errno;
        }

      list_add_tail(&am->actions, &a->node);
      init_debug("Add action %p(%s) to manager %p", a, argv[1], am);
    }
  else
    {
      cmd = calloc(1, sizeof(*cmd));
      if (cmd == NULL)
        {
          init_err("Alloc action command");
          return -errno;
        }

      cmd->argc = init_parse_arguments(buf, true, nitems(cmd->argv) - 1,
                                       cmd->argv);
      if (cmd->argc < 1)
        {
          free(cmd);
          init_err("Invalid command argument");
          return -EINVAL;
        }

      a = list_last_entry(&am->actions, struct action_s, node);
      list_add_tail(&a->cmds, &cmd->node);
      init_debug("Add command %p(%s) to action %p", cmd, cmd->argv[0], a);
      init_dump_args(cmd->argc, cmd->argv);
    }

  return 0;
}

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
void init_dump_actions(FAR struct list_node *head)
{
  FAR struct action_s *action;

  init_debug("== Dump Actions ==");
  list_for_every_entry(head, action, struct action_s, node)
    {
      init_dump_action(action);
    }
}
#endif
