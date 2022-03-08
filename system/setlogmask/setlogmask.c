/****************************************************************************
 * apps/system/setlogmask/setlogmask.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

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

int main(int argc, FAR char *argv[])
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
