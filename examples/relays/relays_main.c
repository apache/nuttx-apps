/****************************************************************************
 * apps/examples/relays/relays_main.c
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
#include <nuttx/arch.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>

#ifdef CONFIG_ARCH_RELAYS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_RELAYS_NRELAYS
#  define CONFIG_EXAMPLES_RELAYS_NRELAYS  2
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: relays_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char *stat = NULL;
  char *no = NULL;
  bool badarg = false;
  bool set_stat = false;
  uint32_t r_stat;
  int option;
  int n = -1;
  int i;

  while ((option = getopt(argc, argv, ":n:")) != ERROR)
    {
      switch (option)
        {
          case 'n':
            no = optarg;
            n = atoi(no);
            break;

          case ':':
            badarg = true;
            break;

          case '?':
          default:
            badarg = true;
            break;
        }
    }

  if (badarg)
    {
      printf("usage: relays [ -n <relay id> ] <switch-status>\n");
      return -1;
    }

  if (optind == argc - 1)
    {
      stat = argv[optind];
      set_stat = (strcmp(stat, "on") == 0 || strcmp(stat, "ON") == 0);
    }

  up_relaysinit();

  if (n >= 0)
    {
      printf("set RELAY ID %d to %s\n", n, set_stat ? "ON" : "OFF");
      relays_setstat(n, set_stat);
    }
  else
    {
      r_stat = 0;
      for (i = 0; i < CONFIG_EXAMPLES_RELAYS_NRELAYS; i++)
        {
          printf("set RELAY ID %d to %s\n", i , set_stat ? "ON" : "OFF");
          r_stat |= (set_stat ? 1 : 0) << i;
        }

      relays_setstats(r_stat);
    }

  return 0;
}

#endif /* CONFIG_ARCH_RELAYS */
