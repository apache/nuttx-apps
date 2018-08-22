/****************************************************************************
 * apps/system/setlogmask/setlogmask.c
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
  printf("\nUsage: %s <d|i|n|w|e|c|a|r>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  d=DEBUG\n");
  printf("  i=INFO\n");
  printf("  n=NOTICE\n");
  printf("  w=WARNING\n");
  printf("  e=ERROR\n");
  printf("  c=CRITICAL\n");
  printf("  a=ALERT\n");
  printf("  r=EMERG\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int setlogmask_main(int argc, char **argv)
#endif
{
  if (argc < 2)
    {
      show_usage(argv[0], EXIT_FAILURE);
    }

  switch (*argv[1])
    {
      case 'd':
        {
          setlogmask(LOG_UPTO(LOG_DEBUG));
        }
        break;
      case 'i':
        {
          setlogmask(LOG_UPTO(LOG_INFO));
        }
        break;
      case 'n':
        {
          setlogmask(LOG_UPTO(LOG_NOTICE));
        }
        break;
      case 'w':
        {
          setlogmask(LOG_UPTO(LOG_WARNING));
        }
        break;
      case 'e':
        {
          setlogmask(LOG_UPTO(LOG_ERR));
        }
        break;
      case 'c':
        {
          setlogmask(LOG_UPTO(LOG_CRIT));
        }
        break;
      case 'a':
        {
          setlogmask(LOG_UPTO(LOG_ALERT));
        }
        break;
      case 'r':
        {
          setlogmask(LOG_UPTO(LOG_EMERG));
        }
        break;
      default:
        {
          show_usage(argv[0], EXIT_FAILURE);
        }
        break;
    }

  return EXIT_SUCCESS;
}
