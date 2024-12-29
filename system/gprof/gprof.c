/****************************************************************************
 * apps/system/gprof/gprof.c
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/gmon.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern uint8_t _stext[];
extern uint8_t _etext[];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      goto help;
    }

  if (strcmp(argv[1], "dump") == 0)
    {
#ifndef CONFIG_DISABLE_ENVIRON
      FAR const char *output = argc < 3 ? "gmon.out" : argv[2];
      setenv("GMON_OUT_PREFIX", output, true);
#else
      fprintf(stderr, "gprof: Environment not supported\n");
#endif
      _mcleanup();
    }
  else if (strcmp(argv[1], "start") == 0)
    {
      monstartup((uintptr_t)&_stext, (uintptr_t)&_etext);
      moncontrol(1);
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      moncontrol(0);
    }
  else
    {
      goto help;
    }

  return EXIT_SUCCESS;

help:
  printf("Usage: gprof <command> [<args>]\n"
        "  gprof dump [output]\n"
        "  gprof start\n"
        "  gprof stop\n");
  return EXIT_FAILURE;
}

