/****************************************************************************
 * apps/testing/irtest/main.cxx
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

#include <stdio.h>

#include "system/readline.h"
#include "cmd.hpp"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t prompt(const char *p, char *buf, size_t len)
{
  fputs(p, stdout);
  fflush(stdout);
  return readline(buf, len, stdin, stdout);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C"
{
int main(int argc, char *argv[])
{
  char cmdline[512];

  init_device();

  while (prompt("$", cmdline, sizeof(cmdline)))
    {
      const char *name = get_first_arg(cmdline);
      if (name != 0 && *name != 0)
        {
          int res =  -ENOSYS;
          for (int i = 0; g_cmd_table[i]; i++)
            {
              if (strcmp(name, g_cmd_table[i]->name) == 0)
                {
                  res = g_cmd_table[i]->exec();
                  break;
                }
            }

          if (res < 0)
            {
              printf("%s: %s(%d)\n", name, strerror(-res), res);
            }
        }
    }

  return 0;
}
}
