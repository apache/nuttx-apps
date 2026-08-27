/****************************************************************************
 * apps/system/nxinit/test/test_nxinit.h
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

#ifndef __APPS_SYSTEM_NXINIT_TEST_TEST_NXINIT_H
#define __APPS_SYSTEM_NXINIT_TEST_TEST_NXINIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_nxinit_group_setup
 ****************************************************************************/

int test_nxinit_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_nxinit_group_teardown
 ****************************************************************************/

int test_nxinit_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_nxinit_parser_*
 ****************************************************************************/

void test_nxinit_parser_arguments_spaces(FAR void **state);
void test_nxinit_parser_arguments_quoted(FAR void **state);
void test_nxinit_parser_arguments_dashdash_separator(FAR void **state);
void test_nxinit_parser_arguments_long_option(FAR void **state);
void test_nxinit_parser_arguments_truncate(FAR void **state);
void test_nxinit_parser_config_sections(FAR void **state);
void test_nxinit_parser_config_skip_blank_lines(FAR void **state);
void test_nxinit_parser_config_unknown_section(FAR void **state);
void test_nxinit_parser_config_line_too_long(FAR void **state);
void test_nxinit_parser_config_line_crosses_boundary(FAR void **state);
void test_nxinit_parser_config_buffer_crosses_boundary(FAR void **state);

/****************************************************************************
 * Name: test_nxinit_action_*
 ****************************************************************************/

void test_nxinit_action_event_match_exact(FAR void **state);
void test_nxinit_action_event_match_invert(FAR void **state);
void test_nxinit_action_event_match_fnmatch(FAR void **state);
void test_nxinit_action_event_and_semantics(FAR void **state);

/****************************************************************************
 * Name: test_nxinit_service_*
 ****************************************************************************/

void test_nxinit_service_duplicate_conflict(FAR void **state);
void test_nxinit_service_override_replaces_duplicate(FAR void **state);
void test_nxinit_service_args_max_boundary(FAR void **state);

#endif /* __APPS_SYSTEM_NXINIT_TEST_TEST_NXINIT_H */
