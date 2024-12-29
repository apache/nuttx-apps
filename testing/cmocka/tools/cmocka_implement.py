#!/usr/bin/env python3
############################################################################
# apps/testing/cmocka/tools/cmocka_implement.py
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

# -*- coding: utf-8 -*-
import copy
import os
import re
import traceback

import typer

TESTSUITE_TEMPLATE = """
/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <cmocka.h>

/****************************************************************************
 * Name: cmocka_{suite_file}_main
 ****************************************************************************/

int main(int argc, char* argv[])
{

  /* Add Test Cases */
  const struct CMUnitTest {suite_name}[] = {
    cmocka_unit_test_setup_teardown(write case name here, NULL, NULL),
  };

    /* Run Test cases */
    cmocka_run_group_tests({suite_name}, NULL, NULL);

    printf("hello cmocka auto-tests\\n");

    return 0;
}
"""

TESTCASE_TEMPLATE = """
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>


/****************************************************************************
 * Name: {case_file}
 * Description: Testing for scene "describe scene here".
 * The detail test steps are as following:
 * 1. describe step 1 here
 * 2. describe step 2 here
 * 3. describe step 3 here
 ****************************************************************************/

void {case_name}(FAR void **state)
{
    printf("case: {case_name}\\n");
    assert(true);
}
"""


class CmockaGen:
    def __init__(self, path):
        self.path = path
        self.suite_path = None
        self.suite_file = None
        self.suite_name = None
        self.case_path = None
        self.case_file = None
        self.case_name = None

    def check_path(self):
        if not self.path:
            print("request correct path option")
            return 1
        if not os.path.exists(self.path):
            os.makedirs(self.path)
        return 0

    def check_suite_option(self, suite_option):
        if not suite_option:
            return 1
        opts = suite_option.split("::")
        if len(opts) != 2:
            print("suite option must like aaa/bbb/ccc.c::VelaAutoTestSuite")
            return 1
        else:
            self.suite_path = opts[0]
            self.suite_name = opts[1]
            paths = self.suite_path.split("/")
            if not paths[-1].endswith(".c"):
                print("suite option must like aaa/bbb/ccc.c::VelaAutoTestSuite")
                return 1
            else:
                self.suite_file = paths[-1]
                return 0

    def generate_suite(self):
        content = copy.deepcopy(TESTSUITE_TEMPLATE)
        file_without_ext = self.suite_file.replace(".c", "")
        content = content.replace("{suite_file}", file_without_ext)
        content = content.replace("{suite_name}", self.suite_name)
        full_path = os.path.join(self.path, self.suite_path)
        dir_path = os.path.dirname(full_path)
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
        with open(full_path, "w") as fl:
            fl.write(content)

    def check_case_option(self, case_option):
        if not case_option:
            return 1
        opts = case_option.split("::")
        if len(opts) != 2:
            print("case option must like aaa/bbb/ccc.c::test_playback_uv_01")
            return 1
        else:
            self.case_path = opts[0]
            self.case_name = opts[1]
            if not self.case_name.startswith("test"):
                print("case function name start with 'test'")
                return 1
            paths = self.case_path.split("/")
            if not paths[-1].endswith(".c"):
                print("case option must like aaa/bbb/ccc.c::VelaAutoTestcase")
                return 1
            file_parts = paths[-1].split("_")
            pattern = r"[0-9]{2,3}\.c$"
            if file_parts[0] != "test":
                print("case file name must start with 'test'")
                return 1
            elif not re.search(pattern, file_parts[-1]):
                print("case file name must end with '00-99' or '000-999'")
                return 1
            else:
                self.case_file = paths[-1]
                return 0

    def generate_case(self):
        content = copy.deepcopy(TESTCASE_TEMPLATE)
        content = content.replace("{case_file}", self.case_file)
        content = content.replace("{case_name}", self.case_name)
        full_path = os.path.join(self.path, self.case_path)
        dir_path = os.path.dirname(full_path)
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
        with open(full_path, "w") as fl:
            fl.write(content)

    def main(self, suite_option, case_option):
        try:
            if self.check_path():
                return
            if not self.check_suite_option(suite_option):
                self.generate_suite()
                print("generate suite success")
            if not self.check_case_option(case_option):
                self.generate_case()
                print("generate case success")
        except Exception:
            traceback.print_exc()


app = typer.Typer()


@app.command()
def main(
    path: str = typer.Option("", help="where to gnerate suite/case file"),
    suite: str = typer.Option(
        default="",
        help="suite file name and suite function name, path/suite::name, eg aaa/bbb/ccc.c::VelaAutoTestcase",
    ),
    case: str = typer.Option(
        default="",
        help="case file name and case function name, path/case::function, eg ddd/eee/fff.c::test_playback_uv_01",
    ),
):
    """
    :param path: where to gnerate suite/case file
    :param suite: suite file name and suite function name
    :param case: case file name and case function name
    :return:
    """
    gen = CmockaGen(path)
    gen.main(suite, case)


if __name__ == "__main__":
    app()
