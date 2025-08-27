/****************************************************************************
 * apps/system/nxinit/action.h
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

#ifndef __APPS_SYSTEM_NXINIT_ACTION_H
#define __APPS_SYSTEM_NXINIT_ACTION_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/list.h>

#include "parser.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct action_cmd_s
{
  struct list_node node;          /* Command list node */
  int argc;
  FAR char *argv[CONFIG_SYSTEM_NXINIT_ACTION_CMD_ARGS_MAX];
};

struct action_s
{
  struct list_node node;          /* Action list node */
  struct list_node ready_node;    /* Ready list node */

  /* Event trigger */

  FAR const char *event;

  struct list_node cmds;          /* Command header, struct action_cmd_s */
};

struct action_manager_s
{
  struct list_node actions;       /* Action header, struct action_s */
  struct list_node ready_actions; /* Ready header, struct action_s */

  FAR char *events[CONFIG_SYSTEM_NXINIT_ACTION_MANAGER_EVENT_MAX];
  FAR struct action_s *current;

  FAR struct action_cmd_s *running;
  int pid_running;
#if defined(CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW) && \
    CONFIG_SYSTEM_NXINIT_ACTION_WARN_SLOW > 0
  struct timespec time_run;
#endif
  FAR struct service_manager_s *sm;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int  init_action_add_event(FAR struct action_manager_s *am,
                           FAR const char *name);
int  init_action_run_command(FAR struct action_manager_s *am);
void init_action_reap_command(FAR struct action_manager_s *am);
int  init_action_parse(FAR const struct parser_s *parser,
                       bool create, FAR char *buf);
#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
void init_dump_actions(FAR struct list_node *head);
#else
#  define init_dump_actions(h)
#endif
#endif /* __APPS_SYSTEM_NXINIT_ACTION_H */
