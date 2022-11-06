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

#include <builtin/builtin.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cmocka_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const char prefix[] = CONFIG_TESTING_CMOCKA_PROGNAME"_";
  FAR const struct builtin_s *builtin;
  int len = strlen(prefix);
  FAR char *bypass[argc + 1];
  FAR char *cases[argc + 1];
  FAR char *skip[argc + 1];
  int num_bypass = 1;
  int num_cases = 0;
  int num_skip = 0;
  int ret;
  int i;
  int j;

  if (strlen(argv[0]) < len - 1 ||
      strncmp(argv[0], prefix, len - 1))
    {
      return 0;
    }

  memset(cases, 0, sizeof(cases));
  memset(skip, 0, sizeof(skip));
  memset(bypass, 0, sizeof(bypass));

  for (i = 1; i < argc; i++)
    {
      if (strcmp("--case", argv[i]) == 0)
        {
          cases[num_cases++] = argv[++i];
        }
      else if (strcmp("--skip", argv[i]) == 0)
        {
          skip[num_skip++] = argv[++i];
        }
      else
        {
          bypass[num_bypass++] = argv[i];
        }
    }

  cmocka_set_skip_filter(NULL);
  for (i = 0; skip[i]; i++)
    {
      cmocka_set_skip_filter(skip[i]);
    }

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

  return 0;
}
