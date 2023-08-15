/****************************************************************************
 * apps/testing/cmocka/cmocka_main.c
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
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <sys/wait.h>
#include <stdio.h>
#include <syslog.h>

#include <builtin/builtin.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void cm_usage (void)
{
    char *mesg =
        "an elegant unit testing framework for C "
        "with support for mock objects\n"
        "Usage: cmocka [OPTION [ARG]] ...\n"
        " -?, --help       show this help statement\n"
        "     --list       display only the names of testcases "
        "and testsuite,\n"
        "                  don't execute them\n"
        "     --test A     only run cases where name matches A pattern\n"
        "     --skip B     don't run cases where name matches B pattern\n"
        "     --case C     specifies testsuite C to run\n"
        "Example: cmocka --case mm --case sched "
        "--test Test* --skip TestNuttxMm0[123]\n\n";
    printf("%s", mesg);
}

/****************************************************************************
 * cmocka_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const char prefix[] = CONFIG_TESTING_CMOCKA_PROGNAME"_";
  FAR const struct builtin_s *builtin;
  int len = strlen(prefix);
  FAR char *testcase = NULL;
  FAR char *bypass[argc + 1];
  FAR char *cases[argc + 1];
  FAR char *skip = NULL;
  int num_bypass = 1;
  int num_cases = 0;
  int ret;
  int i;
  int j;
  int list_tests = 0;

  if (strlen(argv[0]) < len - 1 ||
      strncmp(argv[0], prefix, len - 1))
    {
      return 0;
    }

  memset(cases, 0, sizeof(cases));
  memset(bypass, 0, sizeof(bypass));

  for (i = 1; i < argc; i++)
    {
      if (strcmp("--list", argv[i]) == 0)
        {
          list_tests = 1;
        }
      else if (strcmp("--test", argv[i]) == 0)
        {
          testcase = argv[++i];
        }
      else if (strcmp("--help", argv[i]) == 0 || strcmp("-?", argv[i]) == 0)
        {
          cm_usage();
          return 0;
        }
      else if (strcmp("--case", argv[i]) == 0)
        {
          cases[num_cases++] = argv[++i];
        }
      else if (strcmp("--skip", argv[i]) == 0)
        {
          skip = argv[++i];
        }
      else
        {
          bypass[num_bypass++] = argv[i];
        }
    }

  cmocka_set_test_filter(NULL);
  cmocka_set_skip_filter(NULL);
  cmocka_set_list_test(list_tests);

  if (list_tests == 0)
    {
      cmocka_set_test_filter(testcase);
      cmocka_set_skip_filter(skip);
    }

  print_message("Cmocka Test Start.");
  for (i = 0; (builtin = builtin_for_index(i)) != NULL; i++)
    {
      if (builtin->main == NULL ||
          strlen(builtin->name) < len ||
          strncmp(builtin->name, prefix, len))
        {
          continue;
        }

      for (j = 0; cases[j]; j++)
        {
          if (strncmp(builtin->name + len,
                cases[j], strlen(cases[j])) == 0)
            {
              break;
            }
        }

      if (j && cases[j] == NULL)
        {
          continue;
        }

      bypass[0] = (FAR char *)builtin->name;
      ret = exec_builtin(builtin->name, bypass, NULL, 0);
      if (ret >= 0)
        {
          waitpid(ret, &ret, WUNTRACED);
        }
    }

  print_message("Cmocka Test Completed.");
  return 0;
}
