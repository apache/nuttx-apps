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
#include <string.h>
#include <spawn.h>
#include <sys/param.h>

#include "builtin.h"
#include "init.h"
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

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int cmd_trigger(FAR struct action_manager_s *am,
                       int argc, FAR char **argv);
static int cmd_start(FAR struct action_manager_s *am,
                     int argc, FAR char **argv);
static int cmd_stop(FAR struct action_manager_s *am,
                    int argc, FAR char **argv);
static int cmd_exec(FAR struct action_manager_s *am,
                    int argc, FAR char **argv);
static int cmd_class_start(FAR struct action_manager_s *am,
                           int argc, FAR char **argv);
static int cmd_class_stop(FAR struct action_manager_s *am,
                          int argc, FAR char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct cmd_map_s g_builtin[] =
{
  {"class_start", 2, 2, cmd_class_start},
  {"class_stop", 2, 2, cmd_class_stop},
  {"exec", 3, 99, cmd_exec},
  {"start", 2, 2, cmd_start},
  {"stop", 2, 2, cmd_stop},
  {"trigger", 2, 2, cmd_trigger},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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

static int cmd_start(FAR struct action_manager_s *am,
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
          return g_builtin[i].func(am, argc, argv);
        }
    }

  ret = posix_spawnp(&pid, argv[0], NULL, NULL, argv, NULL);
  if (ret != 0)
    {
      init_err("Executing command '%s': %d", argv[0], ret);
      init_dump_args(argc, argv);
      return -ret;
    }

  init_debug("Executed command '%s' pid %d", argv[0], pid);
  return pid;
}
