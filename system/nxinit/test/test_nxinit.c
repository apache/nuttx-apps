/****************************************************************************
 * apps/system/nxinit/test/test_nxinit.c
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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_nxinit.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest nxinit_tests[] =
    {
      cmocka_unit_test(test_nxinit_parser_arguments_spaces),
      cmocka_unit_test(test_nxinit_parser_arguments_quoted),
      cmocka_unit_test(test_nxinit_parser_arguments_dashdash_separator),
      cmocka_unit_test(test_nxinit_parser_arguments_long_option),
      cmocka_unit_test(test_nxinit_parser_arguments_truncate),
      cmocka_unit_test(test_nxinit_parser_config_sections),
      cmocka_unit_test(test_nxinit_parser_config_skip_blank_lines),
      cmocka_unit_test(test_nxinit_parser_config_unknown_section),
      cmocka_unit_test(test_nxinit_parser_config_line_too_long),
      cmocka_unit_test(test_nxinit_parser_config_line_crosses_boundary),
      cmocka_unit_test(test_nxinit_parser_config_buffer_crosses_boundary),
      cmocka_unit_test(test_nxinit_action_event_match_exact),
      cmocka_unit_test(test_nxinit_action_event_match_invert),
      cmocka_unit_test(test_nxinit_action_event_match_fnmatch),
      cmocka_unit_test(test_nxinit_action_event_and_semantics),
      cmocka_unit_test(test_nxinit_service_duplicate_conflict),
      cmocka_unit_test(test_nxinit_service_override_replaces_duplicate),
      cmocka_unit_test(test_nxinit_service_args_max_boundary),
    };

  return cmocka_run_group_tests(nxinit_tests, test_nxinit_group_setup,
                                test_nxinit_group_teardown);
}
