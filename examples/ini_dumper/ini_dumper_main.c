/****************************************************************************
 * apps/examples/ini_dumper/ini_dumper_main.c
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

#include <stdio.h>

#include <fsutils/ini.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ini_dump
 *
 * Description:
 *   Prints every value in ini file to stdout
 *
 ****************************************************************************/

static int ini_dump(void *user, const char *section, const char *name,
                    const char *value, int lineno)
{
  printf(" %-6d %-15s %-13s %s\n", lineno, section, name, value);

  /* 1 is OK here, 0 is error */

  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ini_dumper_main()
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  if (argc != 2)
    {
      fprintf(stderr, "usage: %s <ini-file>\n", argv[0]);
      return 1;
    }

  printf("------ --------------- ------------- -------------------------\n");
  printf(" line      section          key                 value\n");
  printf("------ --------------- ------------- -------------------------\n");

  ret = ini_parse(argv[1], ini_dump, NULL);
  printf("------ --------------- ------------- -------------------------\n");
  fprintf(stderr, "ini_parse() exited with %d\n", ret);
  return 0;
}
