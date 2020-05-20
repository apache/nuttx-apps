/****************************************************************************
 * apps/examples/ini_dumper/ini_dumper_main.c
 *
 *   Copyright (C) 2019 Michał Łyszczek. All rights reserved.
 *   Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
