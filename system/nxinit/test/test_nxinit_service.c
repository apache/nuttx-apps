/****************************************************************************
 * apps/system/nxinit/test/test_nxinit_service.c
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

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <cmocka.h>

#include "../parser.h"
#include "../service.h"
#include "test_nxinit.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void service_manager_init(FAR struct service_manager_s *sm)
{
  list_initialize(&sm->services);
}

/* init_service_parse()/init_service_check() free failed/duplicate
 * services internally, but successfully-parsed services in "sm" are
 * left for the caller to release. This mirrors what init_service_refresh()
 * would eventually do for SVC_REMOVE-flagged entries, kept minimal here
 * since these tests never start any service (no pid, no posix_spawn).
 */

static void service_manager_free_all(FAR struct service_manager_s *sm)
{
  FAR struct service_s *s;
  FAR struct service_s *tmp;
  FAR struct service_class_s *c;
  FAR struct service_class_s *ctmp;
  int i;

  list_for_every_entry_safe(&sm->services, s, tmp, struct service_s, node)
    {
      list_for_every_entry_safe(&s->classes, c, ctmp,
                                struct service_class_s, node)
        {
          list_delete(&c->node);
          free(c);
        }

      for (i = 0; i < s->argc; i++)
        {
          free(s->argv[i]);
        }

      list_delete(&s->node);
      free(s);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nxinit_service_duplicate_conflict
 *
 * Description:
 *   Two "service" sections sharing the same name, with no "override"
 *   option, are rejected by init_service_check() with -EEXIST.
 ****************************************************************************/

void test_nxinit_service_duplicate_conflict(FAR void **state)
{
  struct service_manager_s sm;
  struct parser_s parser =
    {
      "service", init_service_parse, init_service_check, &sm
    };

  char decl1[] = "service foo /bin/foo";
  char decl2[] = "service foo /bin/bar";

  service_manager_init(&sm);

  assert_int_equal(init_service_parse(&parser, true, decl1), 0);
  assert_int_equal(init_service_parse(&parser, true, decl2), 0);

  assert_int_equal(init_service_check(&parser), -EEXIST);

  service_manager_free_all(&sm);
}

/****************************************************************************
 * Name: test_nxinit_service_override_replaces_duplicate
 *
 * Description:
 *   When the later "service" definition carries the "override" option,
 *   init_service_check() disables/removes the earlier one instead of
 *   failing.
 ****************************************************************************/

void test_nxinit_service_override_replaces_duplicate(FAR void **state)
{
  struct service_manager_s sm;
  struct parser_s parser =
    {
      "service", init_service_parse, init_service_check, &sm
    };

  char decl1[] = "service foo /bin/foo";
  char decl2[] = "service foo /bin/bar";
  char opt_override[] = "  override";
  FAR struct service_s *s1;
  FAR struct service_s *s2;

  service_manager_init(&sm);

  assert_int_equal(init_service_parse(&parser, true, decl1), 0);
  assert_int_equal(init_service_parse(&parser, true, decl2), 0);
  assert_int_equal(init_service_parse(&parser, false, opt_override), 0);

  assert_int_equal(init_service_check(&parser), 0);

  s1 = list_first_entry(&sm.services, struct service_s, node);
  s2 = list_last_entry(&sm.services, struct service_s, node);

  /* SVC_REMOVE sets bit 31, so the raw "int" OR of the two flags is
   * negative; cast explicitly to uint32_t before comparing so
   * assert_int_equal()'s intmax_t widening does not sign-extend one
   * side and zero-extend the other.
   */

  assert_int_equal(s1->flags & (SVC_DISABLED | SVC_REMOVE),
                   (uint32_t)(SVC_DISABLED | SVC_REMOVE));
  assert_int_equal(s2->flags & (SVC_DISABLED | SVC_REMOVE), 0);

  service_manager_free_all(&sm);
}

/****************************************************************************
 * Name: test_nxinit_service_args_max_boundary
 *
 * Description:
 *   A "service" declaration with exactly
 *   CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX tokens is fully captured, while
 *   one more token than the limit is folded into the last slot instead
 *   of overflowing argv.
 ****************************************************************************/

void test_nxinit_service_args_max_boundary(FAR void **state)
{
  /* "service" + "<name>" + "<pathname>" already consume 3 of the
   * CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX tokens, leaving this many
   * "argN" tokens to reach the limit exactly. At the Kconfig lower
   * bound of 3 this is 0, i.e. "service <name> <pathname>" already
   * fills argv on its own with no room for even a single "argN"
   * token, so there is nothing meaningful left to exercise here.
   */

#define NARGS_AT_LIMIT (CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX - 3)

#if NARGS_AT_LIMIT < 1
  skip();
#else
  struct service_manager_s sm;
  struct parser_s parser =
    {
      "service", init_service_parse, init_service_check, &sm
    };

  char at_limit[32 + 8 * NARGS_AT_LIMIT] = "service foo1 /bin/foo";
  char over_limit[32 + 8 * (NARGS_AT_LIMIT + 1)] = "service foo2 /bin/foo";
  char last_arg[16];
  char last_two_args[32];
  FAR struct service_s *s;
  int i;

  for (i = 1; i <= NARGS_AT_LIMIT + 1; i++)
    {
      if (i <= NARGS_AT_LIMIT)
        {
          snprintf(at_limit + strlen(at_limit),
                  sizeof(at_limit) - strlen(at_limit), " arg%d", i);
        }

      snprintf(over_limit + strlen(over_limit),
              sizeof(over_limit) - strlen(over_limit), " arg%d", i);
    }

  snprintf(last_arg, sizeof(last_arg), "arg%d", NARGS_AT_LIMIT);
  snprintf(last_two_args, sizeof(last_two_args), "arg%d arg%d",
          NARGS_AT_LIMIT, NARGS_AT_LIMIT + 1);

  service_manager_init(&sm);

  /* A declaration with exactly CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX
   * tokens is fully captured.
   */

  assert_int_equal(init_service_parse(&parser, true, at_limit), 0);

  s = list_last_entry(&sm.services, struct service_s, node);
  assert_int_equal(s->argc, CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX);
  assert_string_equal(s->argv[CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX - 1],
                      last_arg);

  /* One token beyond the limit is folded into the last argv slot
   * together with the remaining, still-unsplit text, instead of
   * overflowing argv.
   */

  assert_int_equal(init_service_parse(&parser, true, over_limit), 0);

  s = list_last_entry(&sm.services, struct service_s, node);
  assert_int_equal(s->argc, CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX);
  assert_string_equal(s->argv[CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX - 1],
                      last_two_args);

  service_manager_free_all(&sm);
#endif

#undef NARGS_AT_LIMIT
}
