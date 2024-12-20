/****************************************************************************
 * apps/interpreters/minibasic/script.c
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
 *
 * This file was taken from Mini Basic, versino 1.0 developed by Malcolm
 * McLean, Leeds University.  Mini Basic version 1.0 was released the
 * Creative Commons Attribution license which, from my reading, appears to
 * be compatible with the NuttX BSD-style license:
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "interpreters/minibasic.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_INTERPRETER_MINIBASIC_TESTSCRIPT
/* Here is a simple script to play with */

static FAR char *script =
  "10 REM Test Script\n"
  "20 REM Tests the Interpreter\n"
  "30 REM By Malcolm Mclean\n"
  "35 PRINT \"HERE\"\n"
  "40 PRINT INSTR(\"FRED\", \"ED\", 4)\n"
  "50 PRINT VALLEN(\"12a\"), VALLEN(\"xyz\")\n"
  "60 LET x = SQRT(3.0) * SQRT(3.0)\n"
  "65 LET x = INT(x + 0.5)\n"
  "70 PRINT MID$(\"1234567890\", x, -1)\n";
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: leftstring
 *
 * Description:
 *   Function to slurp in an ASCII file
 *   Params: path - path to file
 *   Returns: malloced string containing whole file
 *
 ****************************************************************************/

static FAR char *loadfile(FAR char *path)
{
  FILE *fp;
  int ch;
  long i = 0;
  long size = 0;
  FAR char *answer;

  fp = fopen(path, "r");
  if (!fp)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", path, errno);
      return 0;
    }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  answer = malloc(size + 100);
  if (!answer)
    {
      fprintf(stderr, "ERROR: Out of memory\n");
      fclose(fp);
      return 0;
    }

  while ((ch = fgetc(fp)) != EOF)
    {
      answer[i++] = ch;
    }

  answer[i++] = 0;

  fclose(fp);
  return answer;
}

/****************************************************************************
 * Name: leftstring
 *
 * Description:
 *
 *
 ****************************************************************************/

static void usage(void)
{
  fprintf(stderr, "MiniBasic: a BASIC interpreter\n");
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "Basic <script>\n");
  fprintf(stderr, "See documentation for BASIC syntax.\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main/basic_main
 *
 * Description:
 *   Call with the name of the Minibasic script file
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *scr;

  if (argc == 1)
    {
#ifdef CONFIG_INTERPRETER_MINIBASIC_TESTSCRIPT
      basic(script, stdin, stdout, stderr);
#else
      fprintf(stderr, "ERROR: Missing argument.\n");
      usage();
#endif
    }
  else if (argc == 2)
    {
      scr = loadfile(argv[1]);
      if (scr)
        {
          basic(scr, stdin, stdout, stderr);
          free(scr);
        }
    }
  else
    {
      fprintf(stderr, "ERROR: Too many arguments.\n");
      usage();
    }

  return 0;
}
