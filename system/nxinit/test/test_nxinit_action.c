/****************************************************************************
 * apps/system/nxinit/test/test_nxinit_action.c
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
#include <nuttx/list.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <cmocka.h>

#include "../action.h"
#include "test_nxinit.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Build an empty action manager, ready to accept "on <event>" sections
 * through init_action_parse().
 */

static void action_manager_init(FAR struct action_manager_s *am)
{
  memset(am, 0, sizeof(*am));
  list_initialize(&am->actions);
  list_initialize(&am->ready_actions);
  am->pid_running = -1;
}

/* Return true if any action is currently on the ready queue. */

static bool action_manager_has_ready(FAR struct action_manager_s *am)
{
  return !list_is_empty(&am->ready_actions);
}

/* init_action_parse()/parse_event() strdup() each event's key/value and
 * calloc() both the action and its commands (whose argv entries are in
 * turn strdup'd via init_parse_arguments(..., true, ...)). None of that
 * is released by the code under test itself (init never tears down its
 * action manager), so release it here, mirroring
 * service_manager_free_all() in test_nxinit_service.c.
 */

static void action_manager_free_all(FAR struct action_manager_s *am)
{
  FAR struct action_s *a;
  FAR struct action_s *atmp;
  FAR struct action_cmd_s *cmd;
  FAR struct action_cmd_s *cmdtmp;
  size_t i;
  int j;

  list_for_every_entry_safe(&am->actions, a, atmp, struct action_s, node)
    {
      for (i = 0; i < nitems(a->events); i++)
        {
          if (a->events[i].key)
            {
              free((FAR void *)a->events[i].key);
            }

          if (a->events[i].value)
            {
              free(a->events[i].value);
            }
        }

      list_for_every_entry_safe(&a->cmds, cmd, cmdtmp,
                                struct action_cmd_s, node)
        {
          for (j = 0; j < cmd->argc; j++)
            {
              free(cmd->argv[j]);
            }

          list_delete(&cmd->node);
          free(cmd);
        }

      list_delete(&a->node);
      free(a);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nxinit_action_event_match_exact
 *
 * Description:
 *   An "on property:key=value" action becomes ready only when the exact
 *   key/value pair is triggered.
 ****************************************************************************/

void test_nxinit_action_event_match_exact(FAR void **state)
{
  struct action_manager_s am;
  struct parser_s parser =
    {
      "on", init_action_parse, NULL, &am
    };

  char section[] = "on property:sys.boot.reason=bootloader";
  char cmd[] = "  trigger done";

  action_manager_init(&am);

  assert_int_equal(init_action_parse(&parser, true, section), 0);
  assert_int_equal(init_action_parse(&parser, false, cmd), 0);

  assert_false(action_manager_has_ready(&am));

  init_action_trigger_event(&am, "sys.boot.reason", "other");
  assert_false(action_manager_has_ready(&am));

  init_action_trigger_event(&am, "sys.boot.reason", "bootloader");
  assert_true(action_manager_has_ready(&am));

  action_manager_free_all(&am);
}

/****************************************************************************
 * Name: test_nxinit_action_event_match_invert
 *
 * Description:
 *   An "on property:key!=value" action becomes ready when the value
 *   triggered is anything other than the configured one.
 ****************************************************************************/

void test_nxinit_action_event_match_invert(FAR void **state)
{
  struct action_manager_s am;
  struct parser_s parser =
    {
      "on", init_action_parse, NULL, &am
    };

  char section[] = "on property:sys.boot.reason!=bootloader";
  char cmd[] = "  trigger done";

  action_manager_init(&am);

  assert_int_equal(init_action_parse(&parser, true, section), 0);
  assert_int_equal(init_action_parse(&parser, false, cmd), 0);

  init_action_trigger_event(&am, "sys.boot.reason", "bootloader");
  assert_false(action_manager_has_ready(&am));

  init_action_trigger_event(&am, "sys.boot.reason", "coldboot");
  assert_true(action_manager_has_ready(&am));

  action_manager_free_all(&am);
}

/****************************************************************************
 * Name: test_nxinit_action_event_match_fnmatch
 *
 * Description:
 *   Event values support fnmatch() wildcards, e.g. "bootloader*" matches
 *   any value with that prefix.
 ****************************************************************************/

void test_nxinit_action_event_match_fnmatch(FAR void **state)
{
  struct action_manager_s am;
  struct parser_s parser =
    {
      "on", init_action_parse, NULL, &am
    };

  char section[] = "on property:sys.boot.reason=bootloader*";
  char cmd[] = "  trigger done";

  action_manager_init(&am);

  assert_int_equal(init_action_parse(&parser, true, section), 0);
  assert_int_equal(init_action_parse(&parser, false, cmd), 0);

  init_action_trigger_event(&am, "sys.boot.reason", "recovery");
  assert_false(action_manager_has_ready(&am));

  init_action_trigger_event(&am, "sys.boot.reason", "bootloader_ota");
  assert_true(action_manager_has_ready(&am));

  action_manager_free_all(&am);
}

/****************************************************************************
 * Name: test_nxinit_action_event_and_semantics
 *
 * Description:
 *   When an action has multiple events ("on evA && evB"), it only
 *   becomes ready once every event has been satisfied, not just the
 *   most recently triggered one.
 ****************************************************************************/

void test_nxinit_action_event_and_semantics(FAR void **state)
{
#if CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX > 1
  struct action_manager_s am;
  struct parser_s parser =
    {
      "on", init_action_parse, NULL, &am
    };

  char section[] =
    "on property:sys.boot.reason=bootloader && property:sys.net.ready=1";
  char cmd[] = "  trigger done";

  action_manager_init(&am);

  assert_int_equal(init_action_parse(&parser, true, section), 0);
  assert_int_equal(init_action_parse(&parser, false, cmd), 0);

  init_action_trigger_event(&am, "sys.boot.reason", "bootloader");
  assert_false(action_manager_has_ready(&am));

  init_action_trigger_event(&am, "sys.net.ready", "1");
  assert_true(action_manager_has_ready(&am));

  action_manager_free_all(&am);
#else
  /* Multi-event actions ("on evA && evB") need at least 2 event slots
   * per action, gated the same way builtin.c gates "setprop". With the
   * default CONFIG_SYSTEM_NXINIT_ACTION_EVENTS_MAX=1 there is nothing
   * to exercise here.
   */

  skip();
#endif
}
