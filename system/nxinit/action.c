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
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <fnmatch.h>
#include <nuttx/clock.h>

#include "action.h"
#include "builtin.h"
#include "init.h"
#include "property.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ACTION_SECTION_PROP_PREFIX "property:"
#define ACTION_SECTION_AND_PREFIX  "&&"

#define ACTION_PROP_KEY_DEFAULT    "default"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct event_arg_s
{
  FAR const char *key;
  FAR const char *value;
};

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
  size_t i;

  init_debug("Action %p", action);
  for (i = 0; i < nitems(action->events); i++)
    {
      if (!action->events[i].key)
        {
          continue;
        }

      init_debug("  %s%s%s", action->events[i].key,
                 action->events[i].invert ? "!=" : "==",
                 action->events[i].value);
    }

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
          init_debug("Event %p(%s:%s) already on the queue", a,
                     a->events[0].key, a->events[0].value);
          init_dump_action(a);
          return;
        }
    }

  list_add_tail(&am->ready_actions, &a->ready_node);
}

static int parse_event(FAR char *buf, FAR struct action_event_s *events,
                       size_t count)
{
  FAR char *value;
  FAR char *key;
  size_t i;

  value = strchr(buf, '=');
  if (!value || !*(value + 1))
    {
      return -EINVAL;
    }

  *(value++) = '\0';
  key = strchr(buf, ':');
  if (!key || !*(key + 1))
    {
      return -EINVAL;
    }

  *(key++) = '\0';
  for (i = 0; i < count; i++)
    {
      if (events[i].key)
        {
          continue;
        }

      if (key[strlen(key) - 1] == '!')
        {
          events[i].invert = true;
          key[strlen(key) - 1] = '\0';  /* Skip '!' */
        }
      else
        {
          events[i].invert = false;
        }

      events[i].key = strdup(key);
      events[i].value = strdup(value);
      events[i].pending = false;

      if (events[i].key && events[i].value)
        {
          init_debug("Added action event key:%s value:%s", events[i].key,
                     events[i].value);
          return 0;
        }

      while (i-- > 0)
        {
          if (events[i].key)
            {
              free((FAR void *)events[i].key);
              events[i].key = NULL;
            }

          if (events[i].value)
            {
              free(events[i].value);
            }
        }

      return -errno;
    }

  init_err("Dropped action event key:%s value:%s", key, value);
  return -ENOBUFS;
}

static int event_callback(FAR struct action_manager_s *am,
                          FAR struct action_s *a,
                          FAR struct action_event_s *event,
                          FAR void *argument)
{
  struct event_arg_s *arg = argument;

  if (!strcmp(arg->key, event->key))
    {
      if (event->invert != fnmatch(event->value, arg->value, 0))
        {
          event->pending = false;
        }
      else if (!event->pending)
        {
          event->pending = true;
          init_debug("Trigger %s%s%s", event->key,
                     event->invert ? "!=" : "==",
                     event->value);
          return EVENT_STATE_TRIGGERED;
        }
    }

  return event->pending;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_action_foreach_event(FAR struct action_manager_s *am,
                              init_action_event_cb cb,
                              FAR void *arg)
{
  FAR struct action_s *a;
  size_t i;
  size_t m;
  int ret = 0;

  list_for_every_entry(&am->actions, a, struct action_s, node)
    {
      for (i = 0, m = 1; i < nitems(a->events) && a->events[i].key; i++)
        {
          ret = cb(am, a, &a->events[i], arg);
          if (ret < 0)
            {
              break;
            }

          m = MIN(m * ret, EVENT_STATE_TRIGGERED);
        }

      if (m == EVENT_STATE_TRIGGERED)
        {
          add_ready(am, a);
        }
    }

  return ret;
}

void init_action_trigger_event(FAR struct action_manager_s *am,
                               FAR const char *key,
                               FAR const char *value)
{
  struct event_arg_s arg;

  arg.key = key;
  arg.value = value;
  init_action_foreach_event(am, event_callback, &arg);
}

int init_action_add_event(FAR struct action_manager_s *am,
                          FAR const char *event)
{
  return init_property_set(am->prop, ACTION_PROP_KEY_DEFAULT, event);
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

#if defined(CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW) && \
    CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW > 0
  clock_gettime(CLOCK_MONOTONIC, &am->time_run);
#endif
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

#if defined(CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW) && \
    CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW > 0
  struct timespec time;
  int ms;

  clock_gettime(CLOCK_MONOTONIC, &time);
  clock_timespec_subtract(&time, &am->time_run, &time);
  ms = TIMESPEC2MS(time);
  if (ms > CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW)
    {
      if (am->pid_running <= 0)
        {
          init_warn("Command '%s' took %d ms", am->running->argv[0], ms);
        }
      else
        {
          init_warn("Command '%s' pid %d took %d ms", am->running->argv[0],
                    am->pid_running, ms);
        }
    }
#endif

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
  FAR char *argv[2 * CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX];
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

      while (--ret > 0)
        {
          int n;

          if (!strncmp(argv[ret], ACTION_SECTION_PROP_PREFIX,
                       strlen(ACTION_SECTION_PROP_PREFIX)))
            {
              n = parse_event(argv[ret], a->events, nitems(a->events));
              if (n < 0)
                {
                  free(a);
                  return n;
                }
            }
          else if (strncmp(argv[ret], ACTION_SECTION_AND_PREFIX,
                           strlen(ACTION_SECTION_AND_PREFIX)))
            {
              char tmp[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];

              sprintf(tmp, ":%s=%s", ACTION_PROP_KEY_DEFAULT, argv[ret]);
              n = parse_event(tmp, a->events, nitems(a->events));
              if (n < 0)
                {
                  free(a);
                  return n;
                }

              break;
            }
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
