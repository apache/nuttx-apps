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
#include <stdio.h>
#include <syslog.h>
#include <sys/wait.h>
#include <builtin/builtin.h>
#include <regex.h>
#include <cmocka.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void cm_usage(void)
{
    char *mesg =
        "an elegant unit testing framework for C "
        "with support for mock objects\n"
        "Usage: cmocka [OPTION [ARG]] ...\n"
        " -?, --help       show this help statement\n"
        " -l, --list       display only the names of testcases "
        "and testsuite,\n"
        "                  don't execute them\n"
        " -t, --test A     only run cases where case function "
        "name matches A pattern\n"
        " -p, --skip B     don't run cases where case function "
        "name matches B pattern\n"
        " -s, --suite C    only run suites where PROGNAME "
        "matches C pattern\n"
        " -f, --output-path use xml report instead of standard "
        "output\n"
        " -d, --shuffle-seed shuffling test sequence,between "
        "0 and 99999,\n"
        "                   when seed is 0,use time(NULL) as "
        "the seed for \n"
        "                   the random number generator\n"
        "Example: cmocka --suite mm|sched "
        "--test Test* --skip TestNuttxMm0[123]\n\n";
    printf("%s", mesg);
}

static int cm_regexmatch(FAR const char *pattern, FAR const char *str)
{
  regex_t regex;
  int ret;

  ret = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
  if (ret != 0)
    {
      return 0;
    }

  ret = regexec(&regex, str, 0, NULL, 0);
  regfree(&regex);

  return ret == 0;
}

/****************************************************************************
 * cmocka_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const char prefix[] = CONFIG_TESTING_CMOCKA_PROGNAME"_";
  FAR const struct builtin_s *builtin;
  int prefix_len  = strlen(prefix);
  FAR char *testcase = NULL;
  FAR char *bypass[argc + 1];
  FAR char *suite = NULL;
  FAR char *skip = NULL;
  FAR char *xml_path = NULL;
  FAR char *shuffle_seed = NULL;
  int num_bypass = 1;
  int ret;
  int i;
  int list_tests = 0;

  if (strlen(argv[0]) < prefix_len  - 1 ||
      strncmp(argv[0], prefix, prefix_len  - 1))
    {
      return 0;
    }

  memset(bypass, 0, sizeof(bypass));

  for (i = 1; i < argc; i++)
    {
      if (strcmp("--help", argv[i]) == 0 || strcmp("-?", argv[i]) == 0)
        {
          cm_usage();
          return 0;
        }
      else if (strcmp("--list", argv[i]) == 0 || strcmp("-l", argv[i]) == 0)
        {
          list_tests = 1;
        }
      else if (strcmp("--output-path", argv[i]) == 0
               || strcmp("-f", argv[i]) == 0)
        {
          xml_path = argv[++i];
        }
      else if (strcmp("--shuffle-seed", argv[i]) == 0
               || strcmp("-d", argv[i]) == 0)
        {
          shuffle_seed = argv[++i];
        }
      else if (strcmp("--test", argv[i]) == 0 || strcmp("-t", argv[i]) == 0)
        {
          testcase = argv[++i];
        }
      else if (strcmp("--suite", argv[i]) == 0 || strcmp("-s", argv[i]) == 0)
        {
          suite = argv[++i];
        }
      else if (strcmp("--skip", argv[i]) == 0 || strcmp("-p", argv[i]) == 0)
        {
          skip = argv[++i];
        }
      else if (argv[i][0] == '-')
        {
          printf("Unrecognized arguments: %s\nGet more"
                 " infomation by --help\n", argv[i]);
          return 0;
        }
      else
        {
          bypass[num_bypass++] = argv[i];
        }
    }

  cmocka_set_test_filter(NULL);
  cmocka_set_skip_filter(NULL);
  cmocka_set_message_output(CM_OUTPUT_STDOUT);
  cmocka_set_list_test(list_tests);

  if (list_tests == 0)
    {
      if (xml_path != NULL)
        {
          setenv("CMOCKA_XML_FILE", xml_path, 1);
          cmocka_set_message_output(CM_OUTPUT_XML);
        }

      if (shuffle_seed != NULL)
        {
          setenv("CMOCKA_SHUFFLE_SEED", shuffle_seed, 1);
        }

      cmocka_set_test_filter(testcase);
      cmocka_set_skip_filter(skip);
    }

  print_message("Cmocka Test Start.");
  for (i = 0; (builtin = builtin_for_index(i)) != NULL; i++)
    {
      if (builtin->main == NULL ||
          strlen(builtin->name) < prefix_len  ||
          strncmp(builtin->name, prefix, prefix_len))
        {
          continue;
        }

      if (suite != NULL &&
          !cm_regexmatch(suite, builtin->name))
        {
          continue;
        }

      bypass[0] = (FAR char *)builtin->name;
      ret = exec_builtin(builtin->name, bypass, NULL);
      if (ret >= 0)
        {
          waitpid(ret, &ret, WUNTRACED);
        }
    }

  print_message("Cmocka Test Completed.");
  return 0;
}
